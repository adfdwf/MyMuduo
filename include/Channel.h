#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <functional>
#include <memory>
#include "noncopyable.h"
#include "Timestamp.h"
/*
    Channel封装了fd_和感兴趣的events_（如EPOLLIN、EPOLLOUT事件），还为不同的事件（如读事件、写事件、错误事件等）绑定了回调函数
    当事件发生时，根据事件类型调用相应的回调函数
    还绑定了poller返回的具体事件revents_（由底层的事件循环，如epoll检测到的）
*/

class EventLoop;

class Channel : public noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    Channel(EventLoop *loop, int fd);
    ~Channel();

    // 当channel检测到事件发生时，调用handleEvent方法处理事件
    // handleEvent内部调用handleEventWithGuard，其函数内部根据实际发生的事件(revents_)调用相应的回调函数
    void handleEvent(Timestamp recviveTime);
    void setReadCallback(ReadEventCallback cb)
    {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback cb)
    {
        writeCallback_ = std::move(cb);
    }
    void setCloseCallback(EventCallback cb)
    {
        closeCallback_ = std::move(cb);
    }

    void setErrorCallback(EventCallback cb)
    {
        errorCallback_ = std::move(cb);
    }
    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    //set_revents函数在EpollPoller类中，根据活跃的channel设置实际发生的事件
    void set_revents(int revt) { revents_ = revt; }

    // 向Poller上注册、删除fd感兴趣的事件
    void enableReading()
    {
        events_ |= KReadEvent;
        update();
    }

    void disableReading()
    {
        events_ &= ~KReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= KWriteEvent;
        update();
    }

    void disableWriting()
    {
        events_ &= ~KWriteEvent;
        update();
    }

    void disableAll()
    {
        events_ = KNoneEvent;
        update();
    }

    // 检查channel对象的事件状态，是否对写事件感兴趣，是否对读事件感兴趣或者对任何事件都不感兴趣
    bool isWriting() const { return events_ & KWriteEvent; }
    bool isReading() const { return events_ & KReadEvent; }
    bool isNoneEvent() const { return events_ == KNoneEvent; }

    //index_表示channel在Poller中的状态
    int index() const
    {
        return index_;
    }

    void set_index(int idx)
    {
        index_ = idx;
    }
    // 获取所属的EventLoop
    EventLoop *ownerLoop() { return loop_; }
    /*
     从Poller中移除Channel，但是Poller和Channel不能直接关联，所以remove()函数内部调用的是EventLoop类中的removeChannel()函数，
     removeChannel()函数内部调用的是Epollpoller类中的removeChannel()函数（即通过EventLoop类，间接访问EpollPoller类）其内部本质调用的是epoll_ctl()函数
    */
    void remove();

private:
    //把fd感兴趣的事件注册到Poller上，本质调用的是epoll_ctl
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
    static const int KNoneEvent;
    static const int KReadEvent;
    static const int KWriteEvent;

    EventLoop *loop_; // 事件循环，当前的Channel属于那个Eventloop对象
    int fd_;          // Poller监听的文件描述符
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // poller返回的具体发生的事件
    int index_;       // 对应EpollPoller类中Channel的三个状态KNew、KAdded以及KDeleted

    // 弱引用指针，弱引用另外一个对象，可以检查引用的对象是否存在
    std::weak_ptr<void> tie_;
    // 用于标记 Channel 是否绑定了一个外部资源（通过tie_）
    bool tied_;

    // Channel中能获知fd最终感兴趣的事件revents，所以Channel负责具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

#endif // !__CHANNEL_H__
