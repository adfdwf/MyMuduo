#pragma once
#include "noncopyable.h"
#include <functional>
#include <vector>
#include <memory>
#include <string>

class EventLoop;
class EventLoopThread;


//多线程的 one loop peer thread 
class EventLoopThreadPool
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();
    bool started() const { return started_; }
    const std::string name() const { return name_; }

private:
    //相当于mainloop
    EventLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    //用于轮询下一个事件循环(subloop)的索引
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_;
};
