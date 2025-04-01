#include <sys/epoll.h>

#include "../include/Logger.h"
#include "../include/Channel.h"
#include "../include/EventLoop.h"

const int Channel::KNoneEvent = 0;
const int Channel::KReadEvent = EPOLLIN | EPOLLPRI; // 普通读事件或者紧急数据可读事件
const int Channel::KWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

void Channel::handleEvent(Timestamp receiveTime)
{
    // 检查是否绑定了外部对象(TcpConnection的智能指针)
    if (tied_)
    {
        //说明channel与TcpConnection还保持者关系
        std::shared_ptr<void> guard = tie_.lock(); // 获取强引用
        // 如果能够成功获取强引用，说明绑定的对象仍然存在，可以安全地执行回调函数
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else // 没有绑定外部对象，直接调用事件处理逻辑
    {
        handleEventWithGuard(receiveTime);
    }
}

//channel的tie方法什么时候调用？  一个TcpConnection新连接创建的时候，参数obj在调用时传入的是TcpConnection的智能指针
void Channel::tie(const std::shared_ptr<void> &obj)
{
    /*
        通过std::shared_ptr的赋值操作，tie_会共享obj所指向的对象的所有权。
        这意味着只要Channel实例存在，它就会保持对obj所指向对象的引用，防止该对象被提前销毁。
    */
    tie_ = obj;
    tied_ = true;
}

void Channel::remove()
{
    loop_->removeChannel(this);
}

/*
    当改变Channel所表示fd的events事件后，update负责在Poller里面更改fd相应的事件epoll_ctl
*/
void Channel::update()
{
    // 通过channel所属的EventLoop，调用poller相应的方法，注册fs的events事件
    // add code
    loop_->updateChannel(this);
}

//根据实际发生的事件(revent_)调用相应的回调函数
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revenrs:%d\n", revents_);
    // 连接断开，并且fd上没有可读数据
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}
