#include "../include/CurrentThread.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace CurrentThread
{
    /*
        t_cachedTid 是一个线程局部存储变量，每个线程都有自己的独立副本。
        初始化为 0，表示尚未缓存 tid。
    */
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        // 如果 t_cachedTid 为 0，表示尚未缓存当前线程的 tid。
        if (t_cachedTid == 0)
        {
            // 使用 Linux 系统调用 syscall(SYS_gettid) 获取当前线程的 tid,并存储到t_cachedTid
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}
