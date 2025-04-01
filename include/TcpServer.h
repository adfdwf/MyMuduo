#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "EventLoop.h"
#include "noncopyable.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"


#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

class TcpServer : public noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    enum Option
    {
        KNoReusePort,
        KReusePort
    };

    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option = KNoReusePort);
    ~TcpServer();
    void start();                      // 开启服务器监听
    
    /*
        下面五个set函数均由用户(即使用TcpServer类)调用
    */
    void setThreadNum(int numThreads); // 设置底层的subloop数量
    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = std::move(cb); }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

private:
    // 新连接到来后，调用newConnection函数将新连接分发到subloop
    void newConnection(int sockfd, const InetAddress &peerAddr);
    // 连接断开时，将connections_保存的连接移除
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInloop(const TcpConnectionPtr &conn);
    EventLoop *loop_; // baseloop
    const std::string ipPort_;
    const std::string name_;                          // 服务器名字
    std::unique_ptr<Acceptor> acceptor_;              // 运行在mainloop中，任务是监听新的连接
    std::shared_ptr<EventLoopThreadPool> threadPool_; // 其实就是所有的子Reactor
    ConnectionCallback connectionCallback_;           // 连接到达和连接关闭的回调函数
    MessageCallback messageCallback_;                 // 有读写消息时触发的回调函数
    WriteCompleteCallback writeCompleteCallback_;     // 消息发送完成时触发的回调函数
    ThreadInitCallback threadInitCallback_;           // loop线程初始化时触发的回调函数
    std::atomic_int started_;
    int nextConnId_;
    ConnectionMap connections_;                       // 保存所有的连接
};

#endif // !__TCPSERVER_H__
