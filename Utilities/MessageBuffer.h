#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <string.h>

namespace UTIL
{


class MessageBuffer
{
public:
    using ptr = std::shared_ptr<MessageBuffer>;
    using sizeType = std::vector<uint8_t>::size_type;
  
public:
    MessageBuffer() :         
        wPos_(0),
        rPos_(0),
        buffer_(4096) 
    {
    }

    MessageBuffer(sizeType bufferSize) : 
        wPos_(0),
        rPos_(0),
        buffer_(bufferSize) 
    {}

    MessageBuffer(const MessageBuffer& other)
    {
        wPos_ = other.wPos_;
        rPos_ = other.rPos_;
        buffer_ = other.buffer_;
    }

    MessageBuffer(MessageBuffer&& other)
    {
        wPos_ = other.wPos_;
        rPos_ = other.rPos_;
        buffer_ = std::move(other.buffer_);
    }

    void reset()
    {
        wPos_ = 0;
        rPos_ = 0;
    }

    void resize(sizeType bytes)
    {
        buffer_.resize(bytes);
    }

    //获取buffer的起始地址
    uint8_t* getBasePoint()
    {return buffer_.data();}

    uint8_t* getReadPoint()
    {return buffer_.data() + rPos_;}

    uint8_t* getWritePoint()
    {return buffer_.data() + wPos_;}

    //读取了多少字节
    void readCommit(sizeType bytes)
    {rPos_ += bytes;}

    //写入了多少字节
    void writeCommit(sizeType bytes)
    {wPos_ += bytes;}


    sizeType getActiveSize() const
    {return wPos_ - rPos_;}

    sizeType getRemainingSpace() const
    {return buffer_.size() - wPos_;}

    sizeType getBufferSize() const
    {return buffer_.size();}


    //把没有读取的数据放到缓冲区前面
    void normalize()
    {
        if (rPos_)
        {
            if (rPos_ != wPos_)
                memmove(getBasePoint(), getReadPoint(), getActiveSize());
            wPos_ -= rPos_;
            rPos_ = 0;
        }
    }

    void ensureSpace(sizeType bytes)
    {
        if (getRemainingSpace() < bytes) 
        {
            buffer_.resize(buffer_.size() *1.5);
        }
    }

    void write(const void * data, sizeType bytes)
    {
        if (bytes) 
        {
            ensureSpace(bytes);
            memcpy(getWritePoint(), data, bytes);
            writeCommit(bytes);
        }
    }

    std::vector<uint8_t>&& move()
    {
        wPos_ = 0;
        rPos_ = 0;
        return std::move(buffer_);
    }

    MessageBuffer& operator=(const MessageBuffer & right)
    {
        if (this != &right)
        {
            wPos_ = right.wPos_;
            rPos_ = right.rPos_;
            buffer_ = right.buffer_;
        }
        return *this;
    }

    MessageBuffer& operator=(MessageBuffer&& right)
    {
        if (this != &right)
        {
            wPos_ = right.wPos_;
            rPos_ = right.rPos_;
            buffer_ = right.move();
        }

        return *this;
    }
private:
    sizeType wPos_;
    sizeType rPos_;
    std::vector<uint8_t> buffer_;
};     


}