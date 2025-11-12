#pragma once

#include<functional>
#include<string>
#include<memory>
#include<thread>
#include<atomic>
#include <unistd.h>

#include "noncopyable.h"

class Thread : noncopyable
{
    using ThreadFunc = std::function<void()>;
private:
    void setDefaultName();

    std::string name_;
    pid_t tid_;
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    ThreadFunc func_;
    static std::atomic_int numCreated_;
public:
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started(){ return started_; }
    pid_t tid() { return tid_; }
    const std::string &name() { return name_; }
    static int numCreated(){ return numCreated_; }
};