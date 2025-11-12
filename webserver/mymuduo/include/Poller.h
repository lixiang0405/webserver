#include<vector>
#include<unordered_map>

#include "noncopyable.h"
#include "TimeStamp.h"

class Channel;
class EventLoop;

class Poller : noncopyable
{
    using ChannelMap = std::unordered_map<int, Channel*>;
private:
    EventLoop *ownerloop_;
    protected:
    ChannelMap channels_;
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop *loop);
    virtual ~Poller() = default;
    
    virtual TimeStamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updataChannal(Channel *channel) = 0;
    virtual void removeChannal(Channel *channel) = 0;
    
    bool hasChannal(Channel *channel) const;

    static Poller *newDefaultPoller(EventLoop *loop);
};
