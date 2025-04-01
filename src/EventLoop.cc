#include "../include/EventLoop.h"
#include "../include/Logger.h"
#include "../include/Poller.h"
#include "../include/Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>

// 定义了一个线程局部变量 t_loopInThisThread，用于存储当前线程的 EventLoop 实例。
//  防止一个线程创建多个EventLoop,当t_loopInThisThread为空时，创建一个EventLoop对象（one loop per thread）
__thread EventLoop *t_loopInThisThread = nullptr;
// 定义默认的Poller I/O复用接口的超时时间（epoll_wait 的超时时间）
const int KPollTimeMs = 10000;
// 创建eventfd文件描述符，用于线程间通信(用来notify唤醒subloop处理新来的channel)
int createEventFd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop() : looping_(false), quit_(false), callingPendingFunctors_(false), threadId_(CurrentThread::tid()),
                         poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventFd()),
                         wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    // 如果存在一个EventLoop实例，则报错
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exits in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        // 将当前的 EventLoop 实例赋值给 t_loopInThisThread
        t_loopInThisThread = this;
    }
    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    // 设置wakeupChannel_对读写事件不感兴趣(调用disableAll函数)，并从poller的监听中移除，然后关闭对应的文件描述符wakeupFd_
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::quit()
{
    quit_ = true;
    // 如果当前线程不是事件循环线程，调用wakeup()唤醒事件循环线程
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n", this);
    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(KPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作(queueInLoop中添加的)
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping,\n", this);
    looping_ = false;
}

// 该函数保证了cb这个函数对象一定是在其EventLoop线程中被调用。
void EventLoop::runInLoop(Functor cb)
{
    ////如果当前调用runInLoop的线程正好是EventLoop的运行线程，则直接执行此函数
    if (isInLoopThread())
    {
        cb();
    }
    else
    { // 否则调用 queueInLoop 函数
        queueInLoop(cb);
    }
}

// 把cb这个可调用对象保存在EventLoop对象的pendingFunctors_这个数组中
// 然后会在EventLoop::loop函数中执行doPendingFunctors函数，也就是执行pendingFunctors_数组中所以的回调函数
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 如果当前线程不是事件循环线程，
    // || callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

void EventLoop::hasChannel(Channel *channel)
{
    poller_->hasChannel(channel);
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EvenLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true; // 需要执行回调

    // 括号用于上锁 出括号解锁
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}
