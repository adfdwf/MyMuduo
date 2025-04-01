#include "../include/EventLoopThreadPool.h"
#include "../include/EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg) : baseLoop_(baseLoop), name_(nameArg),
                                                                                            started_(false), numThreads_(0),
                                                                                            next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

// 启动线程池,实际上创建创建了 numThreads_ 个EventLoopThread作为子线程,并让每个eventloopthread调用startLoop()
void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;
    
    for (int i = 0; i < numThreads_; i++)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // 底层创建线程，绑定一个新的EventLoop，并返回该loop的地址,用于后续的轮询操作
        loops_.push_back(t->startLoop());
    }

    // 整个服务端只有一个线程，运行着baseloop
    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{

    EventLoop *loop = baseLoop_;

    if (!loops_.empty()) // 通过轮询，获取下一个处理事件的loop
    {
        //round-robin
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}
