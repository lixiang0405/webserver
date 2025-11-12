#include<sys/epoll.h>

#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"


const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT; 

Channel::Channel(EventLoop *loop, int fd)
:fd_(fd), loop_(loop), event_(0), revent_(0), index_(-1), tied_(false)
{
}
Channel::~Channel(){
    loop_->removeChannel(this);
}

void Channel::updata(){
    loop_->updataChannel(this);
}

void Channel::remove(){
    loop_->removeChannel(this);
}

void Channel::tie(const std::shared_ptr<void>& ptr){
    tie_ = ptr;
    tied_ = true;
}

void Channel::handleEvent(TimeStamp receiveTime){
    if(tied_){
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(TimeStamp receiveTime){
    LOG_INFO("Channel (File descriptor:%d, Index:%d) handle event %d\n",fd_ , index_, revent_);
    
    if((revent_ & EPOLLHUP) && !(revent_ & EPOLLIN)){
        if(closeCallback_){
            closeCallback_();
        }
    }

    if(revent_ & EPOLLERR){
        if(errCallback_){
            errCallback_();
        }
    }

    if(revent_ & EPOLLOUT){
        if(writeCallback_){
            writeCallback_();
        }
    }

    if(revent_ & (EPOLLIN | EPOLLPRI)){
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }
}


