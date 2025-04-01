#include "../include/Thread.h"
#include "../include/CurrentThread.h"
#include <semaphore.h>
std::atomic<int> Thread::numCreated_{0};

Thread::Thread(ThreadFunc func, const std::string &name) : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
{
    setDefaultName();
}
Thread::~Thread()
{
    if (started_ && joined_)
    {
        thread_->detach();
    }
}

/*
    通过信号量确保主线程在新线程启动后获取到正确的线程ID。
    主线程和新线程之间的同步通过信号量实现，确保线程启动的顺序。
*/
void Thread::start()
{
    started_ = true;
    sem_t sem;
    // 使用 sem_init 初始化信号量，设置为非共享（false），初始值为 0。
    sem_init(&sem, false, 0);
    // 创建一个新线程，并在新线程启动后获取其线程ID（tid）
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                           {
                                                            tid_ = CurrentThread::tid();
                                                            // 通过 sem_post 信号量通知主线程，新线程已经启动并获取了 tid。
                                                            sem_post(&sem);
                                                            func_(); }));
    // 这里等待获取上面新创建下线程tid值
    // 主线程在新线程启动并获取 tid 后，才继续执行
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}
