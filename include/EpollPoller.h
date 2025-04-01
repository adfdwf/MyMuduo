#ifndef __EPOLLPOLLER_H__
#define __EPOLLPOLLER_H__

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"

class Channel;
class EpollPoller : public Poller
{
public:
    //对应epoll_wait()
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    //下面两个函数对应epoll_ctl

    /// @brief 调用 epoll_ctl，更新 Channel 的事件状态。
    /// @param channel 需要更新的Channel对象
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

private:
    // 初始EventList的长度
    static const int KInitEventListSize = 16;
    using EventList = std::vector<struct epoll_event>;
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    void update(int operation, Channel *channel);       
    EventList events_;          //存放epoll_wait返回的所有发生的事件
    int epollfd_;
};

#endif // !1 __EPOLLPOLLER_H__
