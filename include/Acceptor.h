#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

// 创建监听的套接字（listenfd），该类运行在主线程（mainLoop），主要负责监听并接收连接，接收的连接分发给其他的子Reactor（subLoop）（子线程）。
// Acceptor是TcpServer管理的
// Acceptor 用于 baseLoop 中，对 listenfd 及其相关方法进行封装（监听新连接到达、接受新连接、分发连接给 subLoop 等）
class Acceptor : public noncopyable
{
public:
    // 新连接到来时的回调函数，包含新连接的sockfd和所连接服务器的InetAddress
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();
    //下面的set函数由TcpServer类调用，当有新用户连接时触发的回调函数
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = std::move(cb);
    }
    bool listenning() const { return listenning_; }
    void listen();

private:
    //当listenfd有事件发生了，就是有新用户连接了，触发这个回调函数
    void handleRead();
    EventLoop *loop_;       // Acceptor用的就是用户定义的那个baseloop_，也就是mainloop
    Socket acceptSocket_;   // 服务器监听套接字的文件描述符（listenfd）
    Channel acceptChannel_; // 把acceptSocket_（listenfd）及其感兴趣事件和事件对应的处理函数进行封装。
    // 新连接到达后执行的回调函数，即将新连接通过轮询的方式分给一个subloop
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};