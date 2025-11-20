#include<unistd.h>
#include<errno.h>
#include<string.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "TimeStamp.h"
#include "Channel.h"

const int kNew = -1;
const int kAdd = 1;
const int kDel = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    :Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), event_(kInitSize)
{}

EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

TimeStamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels){
    LOG_DEBUG("func:%s => fd total count:%lu\n", __FUNCTION__, channels_.size());    
    int cnt = ::epoll_wait(epollfd_, &*event_.begin(), static_cast<int>(event_.size()), timeoutMs);
    int err = errno;
    TimeStamp now = TimeStamp::now();
    if(cnt > 0){
        fillActiveChannels(cnt, activeChannels);
        LOG_DEBUG("%d events happend\n", cnt);
        if(cnt == event_.size()){
            event_.resize(event_.size() * 2);
        }
    }else if(cnt == 0){
        LOG_DEBUG("%s timeout\n", __FUNCTION__);
    }else{
        if(err != EINTR){
            LOG_FATAL("EPollPoller::poll error\n");
            errno = err;
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int size, ChannelList *activeChannels) const{
    for(int i = 0; i < size; ++i){
        Channel *channel = static_cast<Channel*>(event_[i].data.ptr);
        channel -> set_revent(event_[i].events);
        activeChannels -> push_back(channel);
    }
}

void EPollPoller::updata(int op, Channel *channel){
    epoll_event event;
    ::memset(&event, 0, sizeof event);

    int fd = channel -> fd();
    event.events = channel -> event();
    event.data.fd = fd;
    event.data.ptr = channel;

    if(::epoll_ctl(epollfd_, op, fd, &event) < 0){
        if(op == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error\n");
        }else{
            LOG_FATAL("epoll_ctl mod/add error\n");
        }
    }
}

void EPollPoller::updataChannal(Channel *channel){
    const int index = channel -> index();
    int fd = channel -> fd();
    LOG_DEBUG("func:%s => fd:%d, event:%d, index:%d\n", __FUNCTION__, fd, channel -> event(), channel -> index());
    if(index == kNew || index == kDel){
        if(index == kNew){
            channels_[fd] = channel;
        }
        channel -> set_index(kAdd);
        updata(EPOLL_CTL_ADD, channel);
    }else{
        if(channel -> isNoneEvent()){
            updata(EPOLL_CTL_DEL, channel);
            channel -> set_index(kDel);
        }else{
            updata(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannal(Channel *channel){
    LOG_DEBUG("func:%s => fd:%d\n", __FUNCTION__, channel -> fd());
    if(channel -> index() == kAdd){
        updata(EPOLL_CTL_DEL, channel);
    }
    channel -> set_index(kNew);
    channels_.erase(channel -> fd());
}