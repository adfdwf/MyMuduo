#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * TcpServer 设置回调 => TcpConnection => Channel => Poller => Channel的回调操作
 */

/*
   封装了一个已建立的TCP连接，以及控制该TCP连接的方法（连接建立和关闭和销毁），
   以及该连接发生的各种事件（读/写/错误/连接）对应的处理函数，以及这个TCP连接的服务端和客户端的套接字地址信息等。
*/

// 对应connection fd
// TcpConnection 用于 subLoop 中，对 connection fd 及其相关方法进行封装（读消息事件、发消息事件、连接关闭事件、错误事件等
class TcpConnection : public noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    enum StateE
    {
        kDisconnected,
        kDisconnecting,
        KConnecting,
        KConnected
    };
    TcpConnection(EventLoop *loop, const std::string nameArg, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);
    ~TcpConnection();
    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == KConnected; }
    // 给客户端返回数据
    void send(std::string &buf);
    void shutdown();
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb) { highWaterMarkCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }
    void connectEstablished(); // 连接建立
    void connectDestroyed();   // 连接销毁
    void setState(StateE state) { state_ = state; }

private:
    // 负责处理Tcp连接的可读事件，将客户端发送来的数据拷贝到用户缓冲区中inputBuffer_，然后再调用connectionCallback_保存的连接建立后的处理函数
    void handleRead(Timestamp receiceTime);
    // 负责处理Tcp连接的可写事件
    void handleWrite();
    // 处理Tcp连接关闭的事件handleClose()
    void handleClose();
    void handleError();
    void sendInLoop(const void *data, size_t len);
    void shutdownInLoop();

    EventLoop *loop_;
    std::string name_;
    std::atomic_int state_; // TcpConnection的状态
    bool reading_;          // 连接是否在监听事件

    std::unique_ptr<Socket> socket_;   // 对应connection fd，用于处理消息
    std::unique_ptr<Channel> channel_; // 把connection fd封装成channel_

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;       // 连接到达和连接关闭的回调函数
    MessageCallback messageCallback_;             // 有读写消息时触发的回调函数
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成时触发的回调函数
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;

    size_t highWaterMark_;

   //代表的是接收用户发送过来的数据
    Buffer inputBuffer_;
    /*
        同样的这也是一个缓冲区不过这个缓冲区是用来保存那些暂时发送不出去的数据，TCP的发送缓冲区也是有大小限制的，
        如果此时无法将数据一次性拷贝到TCP缓冲区当中，
        那么剩余的数据可以暂时保存在我们自己定义的缓冲区当中并将给文件描述对应的写事件注册到对应的Poller当中，
        等到写事件就绪了，在调用回调方法将剩余的数据发送给客户端。
    */
    Buffer outputBuffer_;
};