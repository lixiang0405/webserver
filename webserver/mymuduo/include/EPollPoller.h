#pragma once

#include<vector>
#include<sys/epoll.h>

#include "Poller.h"

class EPollPoller : public Poller
{
    using EventList = std::vector<epoll_event>;
private:
    static const int kInitSize = 16;
    int epollfd_;
    EventList event_;
    void updata(int op, Channel *channel);
    void fillActiveChannels(int size, ChannelList *activeChannels) const;
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    TimeStamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updataChannal(Channel *channel) override;
    void removeChannal(Channel *channel) override;
};