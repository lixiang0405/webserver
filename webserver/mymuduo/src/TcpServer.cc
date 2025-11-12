#include "string.h"

#include "TcpServer.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"

static EventLoop *checkLoopNoNULL(EventLoop *loop){
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name, Option option)
    :loop_(checkLoopNoNULL(loop))
    ,ipPort_(listenAddr.toIpPort())
    ,name_(name)
    ,acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
    ,threadPool_(new EventLoopThreadPool(loop, name_))
    ,started_(0)
    ,nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer(){
    for(auto &connection : connections_){
        TcpConnectionPtr conn(connection.second);
        connection.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}


void TcpServer::setThreadNum(int numThreads){
    numThreads_ = numThreads;
    threadPool_->setNumThreads(numThreads);
}

void TcpServer::start(){
    if(started_.fetch_add(1) == 0){
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr){
    EventLoop *ioLoop = threadPool_->getNextloop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_++);
    std::string name = name_ + buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",
             name_.c_str(), name.c_str(), peerAddr.toIpPort().c_str());
    sockaddr_in localSock;
    socklen_t len = sizeof localSock;
    ::memset(&localSock, 0, sizeof localSock);
    if(::getsockname(sockfd, (sockaddr *)&localSock, &len) < 0){
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(localSock);
    TcpConnectionPtr conn(new TcpConnection(ioLoop, name, sockfd, localAddr, peerAddr));
    connections_[name] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn){
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn){
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
            name_.c_str(), conn->name().c_str());
    EventLoop *ioLoop = threadPool_->getNextloop();
    ioLoop->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    connections_.erase(conn->name());
}

