#ifndef __POLLER_H__
#define __POLLER_H__
#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Channel.h"
#include "Timestamp.h"

class EventLoop;
class Channel;

//管理和操作Channel对象
//监听fd上的事件，并将活动的channel返回给EventLoop
class Poller
{
public:
    //用于存储活动的Channel对象
    using ChannelList = std::vector<Channel *>;
    // 构造函数接受一个 EventLoop* 类型的参数，表示这个 Poller 属于哪个 EventLoop。
    Poller(EventLoop *loop);
    virtual ~Poller() = default;
    //轮询IO事件，判断是否有数据可读、是否可以写入数据、是否发生错误
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    //判断参数channel是否在当前Poller中
    virtual bool hasChannel(Channel *channel) const;
    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel *>;  //<fd,channel>
    ChannelMap channels_;

private:
    //定义Poller属于哪个事件循环EventLoop
    EventLoop *ownerLoop_;
};

#endif // !__POLLER_H__
