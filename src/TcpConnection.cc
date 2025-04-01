#include "../include/TcpConnection.h"
#include "../include/Logger.h"
#include "TcpConnection.h"
#include "../include/Socket.h"
#include "../include/Channel.h"
#include "../include/EventLoop.h"
#include "../include/Callbacks.h"

#include <functional>
#include <errno.h>
#include <string>
#include <unistd.h>

static EventLoop *checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}


TcpConnection::TcpConnection(EventLoop *loop, const std::string nameArg, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : loop_(checkLoopNotNull(loop)), name_(nameArg), state_(KConnecting), reading_(true), socket_(new Socket(sockfd)),
      localAddr_(localAddr), peerAddr_(peerAddr), channel_(new Channel(loop, sockfd)), highWaterMark_(64 * 1024 * 1024) // 64MB
{
    // 下面给channel设置相应的回调的数，poller给channel通知感兴趣事件发生了，channel会回调相应的函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), (int)state_);
}


void TcpConnection::send(std::string& buf)
{
    if (state_ == KConnected)
    {
        if (loop_->isInLoopThread())
        {
            //底层调用的是Linux的write函数
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::shutdown()
{
    if(state_ == KConnected){
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    //说明outputbuffer中的数据已经发送完
    if(!channel_->isWriting()){
        socket_->shutownWtite();
    }
}

void TcpConnection::connectEstablished()
{
    setState(KConnected);
    //防止TcpConnetion销毁时，channel还在执行,所以将channel绑定到TcpConnetion
    channel_->tie(shared_from_this());
    //向poller注册channel的EPOLLIN事件（即新连接建立后，channel要执行读事件）
    channel_->enableReading();
    //新连接建立，执行回调
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if(state_ = KConnected){
        setState(kDisconnected);
        channel_->disableAll();     //把channel感兴趣的事件从poller中删除
        connectionCallback_(shared_from_this());
    }
    channel_->remove();             //把channel从poller中删除
}

// 负责处理Tcp连接的可读事件，它会将客户端发送来的数据从TCP缓冲区（即从套接字）拷贝到用户缓冲区中（inputBuffer_）
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {   
        //已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessagge
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0) // 客户端断开
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::hanleRead");
        handleError();
    }
}

// 它会将客户端发送来的数据从用户缓冲区中（inputBuffer_）拷贝到TCP缓冲区中（即写入套接字中）
void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);              // 处理了n个
            if (outputBuffer_.readableBytes() == 0) // 发送完成
            {
                channel_->disableWriting(); // 不可写了
                if (writeCompleteCallback_)
                {
                    // 唤醒loop对应的thread线程，执行回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop(); // 在当前loop中删除TcpConnection
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行连接关闭的回调
    closeCallback_(connPtr);      // 关闭连接的回调 TcpServer => TcpServer::removeConnection
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    // 使用getsockopt()函数获取套接字的错误状态
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;       //已经写入的字节数
    size_t remaining = len;  // 未发送的数据
    bool faultError = false; // 记录是否产生错误

    // 之前调用过connection的shutdown 不能在发送了
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected,give up writing!");
        return;
    }

    // 如果当前套接字未处于可写状态（!channel_->isWriting()）且输出缓冲区为空
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // 写入失败
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) // 用于非阻塞模式，不需要重新读或者写
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // 表示对端关闭了连接或连接被重置
                {
                    faultError = true;
                }
            }
        }
    }
    // 如果没有发生不可恢复的错误（!faultError）且仍有数据未写入（remaining > 0），则将剩余数据追加到输出缓冲区。
    if (!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的待发送数据的长度
        size_t oldlen = outputBuffer_.readableBytes();
        if (oldlen + remaining >= highWaterMark_ && oldlen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + remaining));
        }
        // 将剩余数据追加到输出缓冲区
        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // 注册channel写事件，否则poller不会向channel通知EPOLLOUT
        }
    }
}

