#pragma once

#include<functional>

#include "Channel.h"
#include "EventLoop.h"
#include "noncopyable.h"
#include "InetAddress.h"
#include "Socket.h"

class Acceptor : noncopyable
{
    using NewConnectionCallback = std::function<void(int, const InetAddress&)>;
private:
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;

    void handleRead();
public:
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuse);
    ~Acceptor();

    void listen();
    bool listenning() const{ return listenning_; }
    void setNewConnectionCallback(const NewConnectionCallback &cb){ newConnectionCallback_ = cb; }
};