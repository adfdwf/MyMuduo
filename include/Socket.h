#pragma once
#include "noncopyable.h"

class InetAddress;
// 该类的作用就是用于管理TCP连接对应的sockfd的生命周期以及提供一些函数来修改sockfd上的选项
class Socket : public noncopyable
{
public:
    explicit Socket(int socketfd) : socketfd_(socketfd)
    {
    }
    ~Socket();
    int fd() const { return socketfd_; }
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);
    void shutownWtite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int socketfd_;
};