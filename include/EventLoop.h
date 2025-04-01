#ifndef __EVENTLOOP_H__
#define __EVEVNTLOOP_H__
#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
// 事件循环类，主要包含Channel类和Poller类
class Channel;
class Poller;

//管理事件循环，处理I/O事件，执行回调函数
class EventLoop : public noncopyable
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();
    //退出事件循环
    void quit();
    //开启事件循环
    void loop();
    Timestamp pollReturnTime() const { return pollReturnTime_; }
    //在当前loop中执行cb
    void runInLoop(Functor cb);
    /*
        把cb放入队列，唤醒loop所在的线程执行cb
        在多线程情况下，保证用户的回调函数一定要在自己的loop中执行
        queueInLoop函数保证把用户的回调函数添加到自己loop的pendingFunctors_中（跨线程操作，需要加锁）
    */
    void queueInLoop(Functor cb);
    // 当mainloop接受一个channel时，通过wakeup()函数唤醒一个subloop
    // 当EventLoop线程处于阻塞状态（如等待epoll_wait返回时,即loop()函数中的代码：pollReturnTime_ = poller_->poll(KPollTimeMs, &activeChannels_);），这个写操作会导致epoll_wait立即返回，从而使线程从阻塞中唤醒。
    void wakeup();
    //更新channel的状态，调用的是Poller中的updateChannel方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel* channel);
    void hasChannel(Channel *channel);
    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    void handleRead();
    void doPendingFunctors();
    using ChannelList = std::vector<Channel *>;
    std::atomic_bool looping_;          //表示事件循环是否正在运行
    std::atomic_bool quit_;             //表示退出loop循环
    std::atomic_bool callingPendingFunctors_;        //表示当前loop是否有需要执行的回调操作
    const pid_t threadId_;              //记录当前loop所在的线程id
    Timestamp pollReturnTime_;          //poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;    //通过poller_将发生的事件返回给EventLoop，EventLoop所管理的Poller
    // eventfd()创建，作用是：当mainloop获取一个新用户的channel时，通过轮询算法选择一个subloop，通过该变量唤醒一个subloop处理channel
    // 由eventfd()创建，用于线程间通信，这个文件描述符会被包装成wakeupChannel_，每个EventLoop都有一个wakeupfd，用于唤醒自己所在的loop
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;        
    ChannelList activeChannels_;                    //Poller返回的所有事件的channel列表
    // Channel *currentActiveChannel_;
    // 存储loop需要执行的所有回调操作，避免本来属于当前线程的回调函数被其他线程调用，应该把这个回调函数添加到属于它所属的线程，等待它属于的线程被唤醒后调用，满足线程安全
    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_;                          //互斥锁，保护上面vector：pendingFunctors_容器的线程安全操作
};

#endif // !__EVENTLOOP_H__
