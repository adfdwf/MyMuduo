#ifndef __THREAD_H__
#define __THREAD_H__
#include "noncopyable.h"
#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>
class Thread : public noncopyable
{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();
    void start();
    void join();
    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }
    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;     //线程的ID
    ThreadFunc func_;   //线程执行的函数
    std::string name_;
    static std::atomic<int> numCreated_; // 统计创建线程的数量（这里主要用于记录线程的名字）
};

#endif // !__THREAD_H__
