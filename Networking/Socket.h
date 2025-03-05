#pragma once

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <queue>
#include "../Utilities/MessageBuffer.h"

namespace NET 
{

#define READ_BLOCK_SIZE 4096

template <typename T>
class Socket : public std::enable_shared_from_this<T>
{
public:
    using ptr = std::shared_ptr<Socket>;
public:
    Socket(boost::asio::ip::tcp::socket && s) :
        socket_(std::move(s)),
        remoteAddress_(socket_.remote_endpoint().address()),
        remotePort_(socket_.remote_endpoint().port()),
        closed_(false),
        closing_(false),
        isWritingAsync_(false),
        recvBuf_(READ_BLOCK_SIZE)
    {}

    ~Socket()
    {
        closed_ = true;
        socket_.close();
    }


    virtual void start() = 0;

    //网络线程不断调用update去发送数据
    virtual bool update()
    {
        if (closed_) 
            return false;

        //isWritingAsync_当前发送缓冲区已经满了
        if (isWritingAsync_ || (sendQueue_.empty() && !closing_)) 
        {
            return true;
        }

        //发送数据 直到缓冲区满/发送完成
        for (; handleQueue(); ) 
        {
        }

        return true;
    }


    boost::asio::ip::address getRemoteAddress() const
    {
        return remoteAddress_;
    }

    uint16_t getRemotePort() const
    {
        return remotePort_;
    }

    bool IsOpen() const { return !closed_ && !closing_; }

    
    //异步读取数据
    void asyncRead()
    {
        if (!IsOpen()) 
        {
            return;
        }

        recvBuf_.normalize();

        socket_.async_read_some(
            boost::asio::buffer(recvBuf_.getWritePoint(), recvBuf_.getRemainingSpace()),
            std::bind(&Socket::readHandlerInternal, 
                this->shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)
        );
    }

    void queuePacket(UTIL::MessageBuffer && packet)
    {
        sendQueue_.push(std::move(packet));
        //asyncProcessSendQueue();
    }


    void closeSocket()
    {
        if (closed_.exchange(true)) 
        {
            return;   
        }

        boost::system::error_code shutdownError;
        if (socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, shutdownError))
            spdlog::info("network", "Socket::CloseSocket: {} errored when shutting down socket: {} ({})", getRemoteAddress().to_string(),
                shutdownError.value(), shutdownError.message());

        onClose();
    }

    //当发送缓冲区里面没有数据的时候 将socket关闭
    void delayCloseSocket()
    {
        if (closing_.exchange(true)) 
        {
            return;
        }

        if (sendQueue_.empty()) 
        {
            closeSocket();
        }
    }
    

    UTIL::MessageBuffer& GetReadBuffer() { return recvBuf_; }
protected:
    virtual void onClose() { }
    virtual void readHandler() = 0;

    //异步处理发送队列
    bool asyncProcessSendQueue()
    {
        if (isWritingAsync_)
            return false;

        isWritingAsync_ = true;

        
        //当socket可写的时候 调用该回调
        socket_.async_write_some(boost::asio::null_buffers(),
         std::bind(&Socket<T>::writeHandlerWrapper,
            this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        return false;
    }


    void setNoDelay(bool enable)
    {
        boost::system::error_code err;
        if (socket_.set_option(boost::asio::ip::tcp::no_delay(enable), err))
            spdlog::error("network", "Socket::SetNoDelay: failed to set_option(boost::asio::ip::tcp::no_delay) for {} - {} ({})",
                getRemoteAddress().to_string(), err.value(), err.message());
    }

private:
    void readHandlerInternal(boost::system::error_code error, size_t transferredBytes)
    {
        if (error)
        {
            closeSocket();
            return;
        }

        recvBuf_.writeCommit(transferredBytes);
        readHandler();
    }

    void writeHandlerWrapper(boost::system::error_code /*error*/, std::size_t /*transferedBytes*/)
    {
        isWritingAsync_ = false;
        handleQueue();
    }


    bool handleQueue()
    {
        if (sendQueue_.empty())
            return false;
        
        UTIL::MessageBuffer& queuedMessage = sendQueue_.front();
        std::size_t bytesToSend = queuedMessage.getActiveSize();

        boost::system::error_code error;
        std::size_t bytesSent = socket_.write_some(
            boost::asio::buffer(
                queuedMessage.getReadPoint(),
                bytesToSend),
                error);
        

        spdlog::info("Socket::handleQueue content={} bytesToSend={}", queuedMessage.getReadPoint(), bytesToSend);
        if (error)
        {
            //当前发送缓冲区已经满了 等待异步发送
            if (boost::asio::error::would_block == error ||
                boost::asio::error::try_again == error)
            {
                spdlog::info("Socket::handleQueue would_block or try_again");
                return asyncProcessSendQueue();
            }


        }
        else if (0 == bytesSent) 
        {
            spdlog::info("Socket::handleQueue bytesSent=0");
            sendQueue_.pop();
            if (closing_ && sendQueue_.empty())
                closeSocket();
            return false;
        }
        else if(bytesSent < bytesToSend)
        {
            spdlog::info("Socket::handleQueue bytesSent < bytesToSend");
            queuedMessage.readCommit(bytesSent);
            return asyncProcessSendQueue();
        }

        spdlog::info("Socket::handleQueue bytesSent == bytesToSend");
        sendQueue_.pop();
        if (closing_ && sendQueue_.empty())
            closeSocket();

        return !sendQueue_.empty();
    }

private:
    boost::asio::ip::tcp::socket socket_;   //socket对象
    boost::asio::ip::address remoteAddress_;    //对端的地址
    uint16_t remotePort_;    //对端的端口
    std::atomic<bool> closed_;
    std::atomic<bool> closing_;
    bool isWritingAsync_;   //当前是否在异步的写数据

    UTIL::MessageBuffer recvBuf_;    //接收缓冲区
    std::queue<UTIL::MessageBuffer> sendQueue_;   //发送缓冲区
};


}


