#include<memory>
#include<vector>

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &name)
    :baseLoop_(baseLoop)
    ,started_(false)
    ,numThreads_(0)
    ,next_(0)
    ,name_(name)
{}
EventLoopThreadPool::~EventLoopThreadPool(){
}

    
void EventLoopThreadPool::start(const ThreadInitCallback &cb){
    started_ = true;
    for(int i = 0; i < numThreads_; ++i){
        char buf[32 + name_.size()];
        snprintf(buf, sizeof buf, "%s:%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }
    if(!numThreads_ && cb){
        cb(baseLoop_);
    }
}
    
std::vector<EventLoop*> EventLoopThreadPool::getAllLoops(){
    if(loops_.empty()){
        return std::vector<EventLoop*>(1, baseLoop_);
    }else return loops_;
}

    
EventLoop *EventLoopThreadPool::getNextloop(){
    if(loops_.empty()) return baseLoop_;
    EventLoop *loop = loops_[next_++];
    if(next_ >= loops_.size()) next_ = 0;
    return loop;
}