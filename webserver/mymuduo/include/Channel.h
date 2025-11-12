#pragma once

#include<functional>
#include<memory>

#include "noncopyable.h"
#include "TimeStamp.h"

class EventLoop;

class Channel : noncopyable 
{
    using ReadEventCallback = std::function<void(TimeStamp)>;
    using EventCallback = std::function<void(void)>;
private:
    void updata();
    void handleEventWithGuard(TimeStamp receiveTime);
    
    const int fd_;
    EventLoop *loop_;
    int event_;
    int revent_;
    int index_;
    
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback errCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
public:
    Channel(EventLoop *loop, int fd);
    ~Channel();

    void set_index(int index){ index_ = index; }
    int index(){ return index_; }

    int fd(){ return fd_; }
    int event() const{ return event_; }
    void set_revent(int revt){ revent_ = revt; }

    void tie(const std::shared_ptr<void>&);

    void handleEvent(TimeStamp receiveTime);
    void remove();

    EventLoop* ownerLoop(){ return loop_; }

    void setReadCallback(ReadEventCallback fc){ readCallback_ = std::move(fc); }
    void setErrorCallback(EventCallback fc){ errCallback_ = std::move(fc); }
    void setCloseCallback(EventCallback fc){ closeCallback_ = std::move(fc); }
    void setWriteCallback(EventCallback fc){ writeCallback_ = std::move(fc); }

    void enableReading(){ event_ |= kReadEvent; updata(); }
    void disableReading(){ event_ &= ~kReadEvent; updata(); }
    void enableWriting(){ event_ |= kWriteEvent; updata(); }
    void disableWriting(){ event_ &= ~kWriteEvent; updata(); }
    void disableAll(){ event_ = kNoneEvent; updata(); }

    bool isNoneEvent() const{ return event_ == kNoneEvent; }
    bool isReading() const{ return event_ & kReadEvent; }
    bool isWriting() const{ return event_ & kWriteEvent; }
};
