#include<errno.h>
#include<sys/sendfile.h>

#include "TcpConnection.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "TimeStamp.h"

static EventLoop *checkLoopNoNull(EventLoop *loop){
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d subLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    :state_(kConnecting)
    ,loop_(checkLoopNoNull(loop))
    ,name_(name)
    ,socket_(new Socket(sockfd))
    ,channel_(new Channel(loop, sockfd))
    ,localAddr_(localAddr)
    ,peerAddr_(peerAddr)
{
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buff){
    if(state_ == kConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(buff.c_str(), buff.size());
        }else{
            loop_->queueInLoop(std::bind(&TcpConnection::sendInLoop, this, buff.c_str(), buff.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len){
    ssize_t nworte = 0;
    size_t  remaining = len;
    bool faultError = false;
    LOG_INFO("TcpConnection::sendInLoop\n");
    if(state_ == kDisconnected){
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0){
        nworte = ::write(channel_->fd(), data, len);
        if(nworte >= 0){
            remaining -= nworte;
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }else{
            if(errno != EWOULDBLOCK){
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }

    if(!faultError && remaining > 0){
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_){
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char *)data + nworte, remaining);
        if(!channel_->isWriting()){
            LOG_DEBUG("???\n");
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown(){
    if(state_ == kConnected){
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop(){
    if(!channel_->isWriting()){
        socket_->shutdownWrite();
    }
}

void TcpConnection::connectEstablished(){
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();
    if(connectionCallback_)
        connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed(){
    setState(kDisconnected);
    channel_->disableAll();
    if(connectionCallback_)
        connectionCallback_(shared_from_this());
    channel_->remove();
}

void TcpConnection::handleRead(TimeStamp receiveTime){
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    LOG_INFO("TcpConnection::handleRead\n");
    if(n > 0){
        if(messageCallback_) messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }else if(n == 0){
        handleClose();
    }else{
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite(){
    if(channel_->isWriting()){
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if(n > 0){
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0){
                channel_->disableWriting();
                if(writeCompleteCallback_){
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if(state_ == kDisconnecting){
                    shutdownInLoop();
                }
            }
        }else{
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }else{
        LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose(){
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d\n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr(shared_from_this());
    if(connectionCallback_)
        connectionCallback_(connPtr);
    if(closeCallback_)
        closeCallback_(connPtr);
}

void TcpConnection::handleError(){
    int optval = 0;
    socklen_t optlen = sizeof optval;
    int err;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0){
        err = errno;
    }else{
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}

void TcpConnection::sendFile(int fileDescriptor, off_t offset, size_t count){
    if(state_ == kConnected){
        if(loop_->isInLoopThread()){
            sentFileInLoop(fileDescriptor, offset, count);
        }else{
            loop_->queueInLoop(std::bind(&TcpConnection::sentFileInLoop, this, fileDescriptor, offset, count));
        }
    }else{
        LOG_ERROR("TcpConnection::sendFile - not connected");
    }
}

void TcpConnection::sentFileInLoop(int fileDescriptor, off_t offset, size_t count){
    ssize_t bytesSent = 0;
    size_t  remaining = count;
    bool faultError = false;
    
    if(state_ == kDisconnected){
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    if(!channel_->isReading() && outputBuffer_.readableBytes() == 0){
        bytesSent = ::sendfile(channel_->fd(), fileDescriptor, &offset, count);
        if(bytesSent >= 0){
            remaining -= bytesSent;
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }else{
            if(errno != EWOULDBLOCK){
                LOG_ERROR("TcpConnection::sendFileInLoop");
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }

    if(!faultError && remaining > 0){
        loop_->queueInLoop(std::bind(&TcpConnection::sentFileInLoop, this, fileDescriptor, offset, count));
    }
}


