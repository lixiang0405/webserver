#pragma once

#include<mutex>
#include<memory>
#include<atomic>
#include<vector>
#include<functional>

#include "noncopyable.h"
#include "TimeStamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

class EventLoop : noncopyable
{
    using Functor = std::function<void()>;
    using ChannelList = std::vector<Channel*>;
private:

    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t threadId_;

    std::unique_ptr<Poller> poller_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::atomic_bool callingPendingFunctors_;
    TimeStamp pollReturnTime_;


    ChannelList activeChannels_;

    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_;

    void handleRead();
    void doPendingFunctors();
public:
    EventLoop();
    ~EventLoop();

    void quit();
    void loop();

    TimeStamp pollReturnTime() const{ return pollReturnTime_; }

    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);

    void wakeup();

    void updataChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    bool isInLoopThread() const{ return threadId_ == CurrentThread::tid(); }
};
