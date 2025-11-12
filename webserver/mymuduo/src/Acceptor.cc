#include <sys/types.h>
#include <sys/socket.h>

#include "Acceptor.h"
#include "Logger.h"

int creatNonblocking(){
    int fd = ::socket(AF_INET, SOCK_CLOEXEC | SOCK_NONBLOCK | SOCK_STREAM, IPPROTO_TCP);
    if(fd < 0){
        LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return fd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuse)
    :loop_(loop)
    ,acceptSocket_(creatNonblocking())
    ,acceptChannel_(loop, acceptSocket_.fd())
    ,listenning_(false)
{
    LOG_INFO("acceptor fd = %d\n", acceptChannel_.fd());
    acceptSocket_.setReuseAddr(reuse);
    acceptSocket_.setReusePort(reuse);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}


void Acceptor::listen(){
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead(){
    InetAddress addr;
    int confd = acceptSocket_.accept(&addr);
    if(confd >= 0){
        if(newConnectionCallback_){
            newConnectionCallback_(confd, addr);
        }else{
            ::close(confd);
        }
    }else{
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}