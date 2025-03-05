#pragma once

#include "../../Networking/Socket.h"
#include <algorithm>
#include <spdlog/spdlog.h>



class PingSocket : public NET::Socket<PingSocket>
{
public:
    using ptr = std::shared_ptr<PingSocket>;
public:
    PingSocket(boost::asio::ip::tcp::socket && s) : 
        NET::Socket<PingSocket>(std::move(s))
    {}

    virtual ~PingSocket() = default;


    void start() override
    {
        //打印出来连接的ip和端口号
        spdlog::info("PingSocket Client connected from {}", this->getRemoteAddress().to_string());

        asyncRead();
    }


    void readHandler() override
    {
        
        UTIL::MessageBuffer& readBuf = this->GetReadBuffer();
        spdlog::info("PingSocket::readHandler size={}", readBuf.getActiveSize());
        UTIL::MessageBuffer msg(readBuf.getActiveSize());
        msg.write(readBuf.getWritePoint(), readBuf.getActiveSize());
        readBuf.readCommit(readBuf.getActiveSize());
        this->queuePacket(std::move(msg));
        asyncRead();
    }


    void onClose() override
    {
        spdlog::info("PingSocket Client disconnect from {}", this->getRemoteAddress().to_string());
    }
};