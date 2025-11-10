#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Thread.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    :loop_(nullptr)
    ,thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    ,exiting_(false)
    ,mutex_()
    ,cond_()
    ,callback_(cb)
{}

EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if(loop_){
        loop_->quit();
        thread_.join();
    }
}

void EventLoopThread::threadFunc(){
    EventLoop loop;
    if(callback_){
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();//感觉可以不需要
    }
    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

EventLoop *EventLoopThread::startLoop(){
    thread_.start();
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this](){ return loop_ != nullptr; });
        loop = loop_;
    }
    return loop;
}