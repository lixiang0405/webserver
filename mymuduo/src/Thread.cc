#include<semaphore.h>

#include "Thread.h"
#include "CurrentThread.h"

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    :name_(name)
    ,tid_(0)
    ,started_(false)
    ,joined_(false)
    ,func_(std::move(func))
{}
    
Thread::~Thread(){
    if(started_ && !joined_){
        thread_->detach();
    }
}

void Thread::start(){
    sem_t sm;
    sem_init(&sm, false, 0);
    started_ = true;
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();
        sem_post(&sm);
        func_();
    }));
    sem_wait(&sm);
}
    
void Thread::join(){
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName(){
    int num = ++numCreated_;
    if(name_.empty()){
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "thread:%d", num);
        name_ = buf;
    }
}