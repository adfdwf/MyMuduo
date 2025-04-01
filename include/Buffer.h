#pragma once
#include <vector>
#include <string>
#include <algorithm>

/*
@code
+-------------------------------+-------------------+-------------------+
| prependable bytes             | readable bytes    | writable bytes    |
| (CONTENT)                     |                   |                   |
+-------------------------------+-------------------+-------------------+
|                               |                   |                   |
0          <=               readerIndex    <=   writerIndex    <=    size

@endcode
*/

// 用户区Buffer
// 客户端写数据=>Tcp缓冲区=>Buffer（对应TcpConnection.h中的inputBuffer_的数据流向）
// Buffer=>Tcp缓冲区=>客户端（对应TcpConnection.h中的outputBuffer_的数据流向）
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;  // 缓冲区头部
    static const size_t KInitalSize = 1024; // 缓冲区读写初始大小
    explicit Buffer(size_t initialSize = KInitalSize) : buffer_(initialSize + kCheapPrepend), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
    {
    }

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writerableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char *peek() const
    {
        return begin() + readerIndex_;
    }

    void retrieve(size_t len)
    {
        // 读取可读缓冲区中的一部分长度，更新readerIndex_的值，还剩从readerIndex_+len到writerIndex_的数据未读
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        // 读取可读缓冲区中的全部内容
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据转成string类型的数据
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWriterableBytes(size_t len)
    {
        if (writerableBytes() < len)
        {
            // 扩容
            makeSpace(len);
        }
    }

    // 把[data,data+len]内存上的数据添加到可写缓存区中
    void append(const char *data, size_t len)
    {
        ensureWriterableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char *beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char *beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 将TCP缓冲区中的数据拷贝到Buffer中（对应TcpConnection.h中的inputBuffer_的数据流向）
    ssize_t readFd(int sockfd, int *saveErrno);
    // 将Buffer中的数据拷贝到TCP缓冲区中（对应TcpConnection.h中的outputBuffer_的数据流向）
    ssize_t writeFd(int sockfd, int *saveErrno);

private:
    char *begin()
    {
        return &*buffer_.begin();
    }

    const char *begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        if (writerableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();                                                  // 保存一下没有读取的数据
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend); // 挪一挪
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};