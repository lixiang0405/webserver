#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include <netinet/tcp.h>

#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

Socket::~Socket(){
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr){
    if(::bind(sockfd_, (sockaddr*)localaddr.getSock(), sizeof(sockaddr_in))){
        LOG_FATAL("bind sockfd:%d fail\n", sockfd_);
    }
}

void Socket::listen(){
    if(::listen(sockfd_, 1024)){
        LOG_FATAL("listen sockfd:%d fail\n", sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr){
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    ::memset(&addr, 0, sizeof addr);
    int confd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(confd >= 0){
        peeraddr->setSock(addr);
    }
    return confd;
}

void Socket::shutdownWrite(){
    if(::shutdown(sockfd_, SHUT_WR)){
        LOG_ERROR("shutdownWrite sockfd:%d fail\n", sockfd_);
    }
}

void Socket::setTcpNoDelay(bool on){
    int val = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &val, sizeof val);
}

void Socket::setReuseAddr(bool on){
    int val = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val);
}

void Socket::setReusePort(bool on){
    int val = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &val, sizeof val);
}

void Socket::setKeepAlive(bool on){
    int val = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof val);
}