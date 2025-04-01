#include "../include/Acceptor.h"
#include "../include/Logger.h"
#include "../include/InetAddress.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <functional>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport) : loop_(loop), acceptSocket_(createNonblocking()),
                                                                                    acceptChannel_(loop,acceptSocket_.fd()),
                                                                                    listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();         // listen
    acceptChannel_.enableReading(); // acceptChannel_注册到Poller，acceptChannel_是监听的channel（封装了监听的fd：acceptSocket_，只对读事件感兴趣）
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        printf("有新链接");
        if (newConnectionCallback_)
        {
            // newConnectionCallback_是TcpServer设置的一个回调函数setNewConnetionCallback，绑定了TcpServer::newConnection，通过 轮询算法，选择一个subloop，分发当前的新客户端的Channel，并且绑定了一些回调。
            newConnectionCallback_(connfd, peerAddr); // 轮询找到subloop唤醒，分发当前的新客户端的Channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == ENFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}
