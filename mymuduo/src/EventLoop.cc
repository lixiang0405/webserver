#include <functional>
#include<sys/eventfd.h>
#include<errno.h>
#include<assert.h>

#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"
#include "Logger.h"

__thread EventLoop *t_loopInThisThread = nullptr;
const int kPollTimeMs = 10000;

int createEventFd(){
    int fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if(fd < 0){
        LOG_FATAL("wakeup fd creat fail:%d\n", errno);
    }
    return fd;
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))
    ,wakeupFd_(createEventFd())
    ,wakeupChannel_(new Channel(this, wakeupFd_))
    ,callingPendingFunctors_(false)
{
    LOG_INFO("EventLoop %p created in thread %d\n", this, threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exited in thread %d\n", t_loopInThisThread, threadId_);
    }else{
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
} 

void EventLoop::quit(){
    quit_ = true;
    if(!isInLoopThread()){
        wakeup();
    }
}

void EventLoop::loop(){
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping\n", this);

    while(!quit_){
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    
        for(auto *channel : activeChannels_){
            channel->handleEvent(pollReturnTime_);
        }

        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping\n", this);
    looping_ = false;
}

void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){
        cb();
    }else{
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lock(mutex_); 
        pendingFunctors_.emplace_back(cb);
    }
    if(!isInLoopThread() || callingPendingFunctors_){
        wakeup();
    }
}

void EventLoop::wakeup(){
    uint64_t a = 1;
    ssize_t n = write(wakeupFd_, &a, sizeof a);
    if(n != sizeof a){
        LOG_ERROR("EventLoop %p write wrong %lu bytes\n", this, n);
    }
}

void EventLoop::updataChannel(Channel *channel){
    poller_->updataChannal(channel);
}
void EventLoop::removeChannel(Channel *channel){
    poller_->removeChannal(channel);
}
bool EventLoop::hasChannel(Channel *channel){
    return poller_->hasChannal(channel);
}

void EventLoop::handleRead(){
    uint64_t a = 1;
    ssize_t n = read(wakeupFd_, &a, sizeof a);
    if(n != sizeof a){
        LOG_ERROR("EventLoop %p read wrong %lu bytes\n", this, n);
    }
}

void EventLoop::doPendingFunctors(){
    std::vector<Functor> funcs;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        funcs.swap(pendingFunctors_);
    }

    for(const auto &func : funcs){
        func();
    }
    callingPendingFunctors_ = false;
}
