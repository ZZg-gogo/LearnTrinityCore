#pragma once
#include "../../Networking/NetworkThread.h"
#include "../../Networking/SocketMgr.h"
#include <spdlog/spdlog.h>
#include "PingSocket.h"

class PingNetworkThread : public NET::NetworkThread<PingSocket>
{

};


inline static void OnSocketAccept(boost::asio::ip::tcp::socket&& sock, uint32_t threadIndex);

class PingSocketMgr : public NET::SocketMgr<PingSocket>
{
public:
    virtual bool startNetwork(boost::asio::io_context& ioc,
        const std::string& ip,
        uint16_t port,
        uint32_t threadCount) override
    {
        if (!NET::SocketMgr<PingSocket>::startNetwork(ioc, ip, port, threadCount)) 
        {
            spdlog::error("Failed to start network");
            return false;
        }
        
        
        pAcceptor_->AsyncAcceptWithCallback<OnSocketAccept>();
        return true;
    }

    NET::NetworkThread<PingSocket>* createThreads()const override
    {
        return new PingNetworkThread[threadCount_];
    }

    static PingSocketMgr* getInstance()
    {
        static PingSocketMgr instance;
        return &instance;
    }

};


inline static void OnSocketAccept(boost::asio::ip::tcp::socket&& sock, uint32_t threadIndex)
{
    PingSocketMgr::getInstance()->onSocketOpen(std::forward<boost::asio::ip::tcp::tcp::socket>(sock), threadIndex);
}