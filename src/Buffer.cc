#include "../include/Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

// 从该TCP接收缓冲区(fd中存储的数据)中将数据读出来并放到Buffer中
ssize_t Buffer::readFd(int sockfd, int *saveErrno)
{

    char extrabuf[65536] = {0}; // 栈上内存空间
    struct iovec vec[2];
    const size_t writable = writerableBytes(); // buffer底层缓冲区剩余的可写的空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(sockfd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // buffer可写的缓冲区已经够存储读取出来的数据
    {
        writerIndex_ += n;
    }
    else // extrabufl里面也写入了数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable); // writerIndex_ 开始写n-writable的数据
    }
    return n;
}

ssize_t Buffer::writeFd(int sockfd, int *saveErrno)
{
    ssize_t n = ::write(sockfd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}