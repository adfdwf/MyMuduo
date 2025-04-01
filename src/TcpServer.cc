#include "../include/TcpServer.h"
#include "../include/Logger.h"
#include "../include/TcpConnection.h"

#include <strings.h>
#include <unistd.h>

static EventLoop *checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option) : loop_(checkLoopNotNull(loop)), ipPort_(listenAddr.toIpPort()), name_(nameArg),
                                                                                                                  acceptor_(new Acceptor(loop, listenAddr, option == KReusePort)), threadPool_(new EventLoopThreadPool(loop, name_)),
                                                                                                                  connectionCallback_(), messageCallback_(), nextConnId_(1),started_(0)
{
    // 当有新用户连接时，会执行TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    // 确保start只启动一次
    if (started_++ == 0)
    {
        // 启动底层的loop线程池，开启所有子线程（子Reactor）此时所有子线程启动后都阻塞在EventLoop的epoller_->poll这里
        threadPool_->start(threadInitCallback_);
        // 开启监听,acceptor_.get()是调用 listen 方法的对象的指针。
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    //通过轮询算法来选择一个subloop来管理channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    LOG_INFO("TcpSerevr::newConnection [%s] - new connection [%s] from %s \n", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    // 通过 sockfd 获取其绑定的本机的 IP 地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof(local));
    socklen_t addrlen = sizeof(local);
    // getsockname()函数用于获取一个套接字的本地地址（即绑定到该套接字的本地 IP 地址和端口号）
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    /*
        下面的回调都是用户设置给TcpServer=>TcpConnection=>Channel=>Poller=>通知channel调用相应操作
    */
    // 设置连接建立时的回调
    conn->setConnectionCallback(connectionCallback_);

    // 设置接收消息时的回调
    conn->setMessageCallback(messageCallback_);

    // 设置写操作完成时的回调
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置如何关闭连接的回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 直接调用 TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInloop,this,conn));
}

void TcpServer::removeConnectionInloop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
