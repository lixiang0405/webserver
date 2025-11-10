#pragma once

/**
 * 用户使用muduo编写服务器程序
 **/

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

class TcpServer
{
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    enum Option{
        kNoReusePort,
        kReusePort,
    };
private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    ConnectionMap connections_;

    EventLoop *loop_;

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;

    int numThreads_;
    std::atomic_int started_;
    int nextConnId_;
public:
    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb){ threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb){ connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb){ messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb){ writeCompleteCallback_ = cb; }

    void setThreadNum(int numThreads);

    void start();
};