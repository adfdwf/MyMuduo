#ifndef __EVENTLOOPTHREAD_H__
#define __EVENTLOOPTHREAD_H__
#include "noncopyable.h"
#include "Thread.h"
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

/*
    封装了一个线程和一个事件循环(EventLoop)，用于在单独的线程中运行事件循环。
    单个线程的 one loop peer thread
*/

class EventLoop;
class EventLoopThread : public noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    // cb和name均提供默认值（即空的std::function 对象和一个空的字符串）
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = std::string());
    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();
    // 当前线程运行的EventLoop
    EventLoop *loop_;
    // 标志线程是否退出
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    // 线程初始化时的回调函数
    ThreadInitCallback callback_;
};

#endif // !__EVENTLOOPTHREAD_H__