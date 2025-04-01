#include "../include/EventLoopThread.h"
#include "../include/EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name) : loop_(nullptr), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(), cond_(), callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}


// 其主要作用就是开启Thread底层的start方法让线程跑起来但是必须保证线程对应的EventLoop对象被创建之后才能获对应线程的EventLoop对象并返回给EventThreadPool
//  负责启动一个新线程，并通过 threadFunc() 在新线程中创建一个独立的 EventLoop。
EventLoop *EventLoopThread::startLoop()
{
    // 启动线程，线程入口函数是threadFunc(构造函数中初始化的)，即每个线程都会创建一个EventLoop对象
    thread_.start();
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            // 条件变量 cond_ 会在 threadFunc 中被触发，表示EventLoop已经创建完成。
            cond_.wait(lock);
        }
        loop = loop_;
    }

    return loop;
}

// 该方法是在单独的新线程里面运行(thread类中start函数（23行代码）中创建的新线程)
// 新线程的入口函数，负责创建 EventLoop 并启动事件循环。
void EventLoopThread::threadFunc()
{
    // 创建一个独立的EventLoop，和新线程一一对应，即one loop peer thread
    EventLoop loop;
    if (callback_)
    {
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    //启动事件循环
    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
