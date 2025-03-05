#pragma once


#include <boost/asio/io_context.hpp>
#include <cstdint>
#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <spdlog/spdlog.h>
#include "AsyncAcceptor.h"
#include "NetworkThread.h"

namespace NET 
{

template<class SocketType>
class SocketMgr
{
public:
    using ptr = std::shared_ptr<SocketMgr>;

public: 
    virtual ~SocketMgr()
    {}

    virtual bool startNetwork(boost::asio::io_context& ioc,
        const std::string& ip,
        uint16_t port,
        uint32_t threadCount)
    {
        AsyncAcceptor* acceptor = nullptr;

        try 
        {
            acceptor = new AsyncAcceptor(ioc, ip, port);
        } 
        catch (boost::system::system_error const& err) 
        {
            spdlog::error("SocketMgr::startNetwork network Exception caught in SocketMgr.StartNetwork ({}:{}): {}", ip, port, err.what());
            return false;
        }

        if (!acceptor->bind()) 
        {
            spdlog::error("network StartNetwork failed to bind socket acceptor");
            delete acceptor;
            return false;
        }

        pAcceptor_ = acceptor;
        threadCount_ = threadCount;
        pNetworkThreads_ = createThreads();


        for (int i = 0; i < threadCount_; ++i) 
        {
            pNetworkThreads_[i].start();
        }

        pAcceptor_->setSockFactory([this](){
            return GetSocketForAccept();
        });

        return true;
    }

    virtual void stopNetwork()
    {
        pAcceptor_->close();
        if (threadCount_ != 0) 
        {
            for (int i = 0; i < threadCount_; ++i) 
            {
                pNetworkThreads_[i].stop();
            }
        }

        wait();

        delete pAcceptor_;
        pAcceptor_ = nullptr;
        delete[] pNetworkThreads_;
        pNetworkThreads_ = nullptr;
        threadCount_ = 0;
    }


    void wait()
    {
        if (threadCount_ != 0) 
        {
            for (int i = 0; i < threadCount_; ++i) 
            {
                pNetworkThreads_[i].wait();
            }
        }

    }

    virtual void onSocketOpen(boost::asio::ip::tcp::socket&& sock, uint32_t threadId)
    {
        try 
        {
            std::shared_ptr<SocketType> sockPtr = std::make_shared<SocketType>(std::move(sock));
            sockPtr->start();
            pNetworkThreads_[threadId].addNewSocket(sockPtr);
        } 
        catch (boost::system::system_error const& err) 
        {
            spdlog::error("SocketMgr::onSocketOpen Exception caught in SocketMgr.onSocketOpen: {}", err.what());
        }
    }


    uint32_t getThreadCount() const
    {
        return threadCount_;
    }

    uint32_t getMinConnectionsThreadId() const
    {
        uint32_t minId = 0;

        for (int i = 0; i < threadCount_; ++i)
        {
            if (pNetworkThreads_[i].getConnsCount() < pNetworkThreads_[minId].getConnsCount())
            {
                minId = i;
            }
        }

        return minId;
    }

    std::pair<boost::asio::ip::tcp::socket*, uint32_t> GetSocketForAccept()
    {
        uint32_t threadIndex = getMinConnectionsThreadId();
        return std::make_pair(pNetworkThreads_[threadIndex].getNewClientSocket(), threadIndex);
    }
protected:
    SocketMgr() :
        pAcceptor_(nullptr),
        pNetworkThreads_(nullptr),
        threadCount_(0)
    {}

    virtual NetworkThread<SocketType>* createThreads() const = 0;

    AsyncAcceptor* pAcceptor_;
    NetworkThread<SocketType>* pNetworkThreads_;
    uint32_t threadCount_;
};


}