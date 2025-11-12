#pragma once

#include<functional>
#include<string>
#include<vector>
#include<memory>

#include "EventLoopThread.h"
#include "EventLoop.h"

class EventLoopThreadPool
{
    using ThreadInitCallback = std::function<void(EventLoop*)>; 
private:
    EventLoop *baseLoop_;
    bool started_;
    int numThreads_;
    int next_;
    std::string name_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
public:
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &name = std::string());
    ~EventLoopThreadPool();

    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    bool started() const{ return started_; }

    std::vector<EventLoop*> getAllLoops();
    const std::string &name() const{ return name_; }
    void setNumThreads(int numThreads){ numThreads_ = numThreads; }
    EventLoop *getNextloop();
};
