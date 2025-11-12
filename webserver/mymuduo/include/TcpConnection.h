#pragma once

#include<memory>
#include<atomic>
#include<string>

#include "noncopyable.h"
#include "TimeStamp.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"

class EventLoop;
class Socket;
class Channel;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
private:
    enum StateE{
        kConnected,
        kConnecting,
        kDisconnecting,
        kDisconnected,
    };
    void setState(StateE state){ state_ = state; }

    void handleRead(TimeStamp receiveTime);
    void handleWrite();
    void handleError();
    void handleClose();

    void sendInLoop(const void *data, size_t len);
    void shutdownInLoop();
    void sentFileInLoop(int fileDescriptor, off_t offset, size_t count);

    std::atomic_int state_;
    EventLoop *loop_;
    const std::string name_;
    bool reading_;
    size_t highWaterMark_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ConnectionCallback connectionCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    MessageCallback messageCallback_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

public:
    TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    void send(const std::string &buff);
    void sendFile(int fileDescriptor, off_t offet, size_t count);

    void shutdown();

    void setMessageCallback(const MessageCallback &cb){ messageCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb){ closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark){ highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb){ writeCompleteCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb){ connectionCallback_ = cb; }

    void connectEstablished();
    void connectDestroyed();
};
