#include "Timestamp.h"
#include "time.h"
#include <sys/time.h>

Timestamp::Timestamp():microSecondsSinceEpoch_(0)
{
}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch) : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{
}
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / 1000000); // 转换为秒
    // int microseconds = static_cast<int>(microSecondsSinceEpoch_ % 1000000);  // 获取微秒部分
    tm *tm_time = localtime(&seconds);
    snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900, // 年份
             tm_time->tm_mon + 1,     // 月份，需要加1
             tm_time->tm_mday,        // 日
             tm_time->tm_hour,        // 小时
             tm_time->tm_min,         // 分钟
             tm_time->tm_sec         // 秒
             );           // 微秒
    return buf;
}
Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);                                             // 获取当前时间，精确到微秒
    int64_t microSecondsSinceEpoch = tv.tv_sec * 1000000LL + tv.tv_usec; // 转换为微秒
    return Timestamp(microSecondsSinceEpoch);
}
Timestamp::~Timestamp()
{
}

// #include <iostream>
// int main(){
//     std::cout<<Timestamp::now().toString();
//     return 0;
// }