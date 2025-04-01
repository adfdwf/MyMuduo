#include "../include/Poller.h"
#include "../include/EpollPoller.h"
#include <stdlib.h>

//Poller中的静态函数，调用这个函数获取具体的I/O复用的对象：EpollPoller对象或PollPoller对象(poll代码中没有实现)，即该函数返回epoll对象
Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    // 如果设置了 MODUO_USE_POLL 环境变量，则表示用户希望使用 poll 机制，而不是默认的 epoll 机制。
    if (::getenv("MODUO_USE_POLL"))
    {
        return nullptr; // 生成poll的实例（代码中只是实现了epoll，没有实现poll，所以这里返回为空）
    }
    else
    {
        return new EpollPoller(loop); // 生成epoll的实例
    }
}
