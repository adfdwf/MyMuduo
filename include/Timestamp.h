#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include <iostream>
#include <string>


class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    std::string toString() const;
    static Timestamp now();
    ~Timestamp();

private:
    // 自纪元（1970年1月1日00:00:00 UTC）以来的微秒数
    int64_t microSecondsSinceEpoch_;
};

#endif