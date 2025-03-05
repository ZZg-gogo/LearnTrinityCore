#pragma once


#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>

namespace NET
{

template <typename SocketType>
class NetworkThread
{
public:
    using ptr = std::shared_ptr<NetworkThread>;
    using SocketContainer = std::vector<std::shared_ptr<SocketType>> ;
public:
    NetworkThread() :
        conns_(),
        newConns_(),
        ioc_(),
        newClientSock_(ioc_),
        connsCount_(0),
        stopped_(false),
        thread_(nullptr),
        updateTimer_(ioc_)
    {}

    ~NetworkThread()
    {
        stop();

        if (thread_) 
        {
            wait();
            delete thread_;
            thread_ = nullptr;
        }
    }



    void stop()
    {
        stopped_ = true;
        ioc_.stop();
    }


    void wait()
    {
        thread_->join();
        delete thread_;
        thread_ = nullptr;
    }


    bool start()
    {
        if (thread_) 
        {
            return false;
        }


        thread_  = new std::thread(std::bind(&NetworkThread::run, this));

        return true;
    }

    uint32_t getConnsCount() const
    {
        return connsCount_;
    }

    virtual void addNewSocket(std::shared_ptr<SocketType> sock)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        newConns_.push_back(sock);
        ++connsCount_;
        socketAdded(sock);
    }

    boost::asio::ip::tcp::socket* getNewClientSocket()
    {
        return &newClientSock_;
    }


protected:
    virtual void socketAdded(std::shared_ptr<SocketType> /*sock*/) { }
    virtual void socketRemoved(std::shared_ptr<SocketType> /*sock*/) { }

    void addNewSockets()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (newConns_.empty()) 
        {
            return;
        }
        
        for (std::shared_ptr<SocketType> sock : newConns_) 
        {
            if (!sock->IsOpen()) 
            {
                this->socketRemoved(sock);
                --connsCount_;
            }
            else
                conns_.push_back(sock);
        }

        newConns_.clear();
    }    


    void run()
    {

        spdlog::info("NetworkThread::run");

        updateTimer_.expires_from_now(std::chrono::milliseconds(1));
        updateTimer_.async_wait([this](boost::system::error_code const&){
            update();
        });

        ioc_.run();

        newConns_.clear();
        conns_.clear();
    }


    /**
    * @brief 定时移出无效的socket 发送写缓冲区里面的数据
    */
    void update()
    {
        if (stopped_) 
        {
            return;
        }

        updateTimer_.expires_from_now(std::chrono::milliseconds(1));
        updateTimer_.async_wait([this](boost::system::error_code const&){
            update();
        });

        addNewSockets();


        conns_.erase(std::remove_if(conns_.begin(), conns_.end(), [this](std::shared_ptr<SocketType> sock){
            //这里判断socket是否断开 未断开进行数据的发送
            if (!sock->update()) 
            {
                if (sock->IsOpen()) 
                {
                    sock->closeSocket();
                }

                this->socketRemoved(sock);
                --connsCount_;
                
                return true;
            }

            return false;
        }), conns_.end());
    }

private:
    std::mutex mutex_;


    SocketContainer conns_;
    SocketContainer newConns_;  //新连接s
    boost::asio::io_context ioc_;   //one thread one loop
    boost::asio::ip::tcp::socket newClientSock_;    //新连接的socket

    std::atomic<uint32_t> connsCount_;  //连接数
    std::atomic<bool> stopped_;

    std::thread* thread_; //one thread one loop
    
    boost::asio::steady_timer updateTimer_; //更新定时器
};


}