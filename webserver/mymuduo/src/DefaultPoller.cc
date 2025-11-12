#include<stdlib.h>

#include "EPollPoller.h"
#include "EventLoop.h"

Poller *Poller::newDefaultPoller(EventLoop *loop){
    if(::getenv("MUDDO_USE_POLL")){
        return nullptr;
    }else{
        return new EPollPoller(loop);
    }
}