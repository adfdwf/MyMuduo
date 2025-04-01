#ifndef __CURRENTTHREAD_H__
#define __CURRENTTHREAD_H__

// 用于获取和缓存当前线程的线程ID（tid），对外使用tid()函数获取当前线程的ID
namespace CurrentThread
{
    // 线程局部变量，用于缓存当前线程的线程ID，__thread 是 GCC 的扩展语法，表示该变量是线程局部的。每个线程都会有一个独立的 t_cachedTid 变量。
    extern __thread int t_cachedTid;
    void cacheTid();
    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}

#endif // !__CURRENTTHREAD_H__
