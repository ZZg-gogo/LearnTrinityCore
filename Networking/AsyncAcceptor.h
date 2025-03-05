#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <utility>
#include <spdlog/spdlog.h>

namespace NET 
{

class AsyncAcceptor
{
public:
    using ptr = std::shared_ptr<AsyncAcceptor>;
    using AcceptCallback = void(*)(boost::asio::ip::tcp::socket&& newSocket, uint32_t threadIndex);

public:

    AsyncAcceptor(boost::asio::io_context& ioContext, const std::string& ip ,uint16_t port) :
        acceptor_(ioContext),
        endpoint_(boost::asio::ip::make_address(ip), port),
        socket_(ioContext),
        closed_(false),
        socketFactory_(std::bind(&AsyncAcceptor::defeaultSocketFactory, this))
    {

    }


    template<class T>
    void AsyncAccept()
    {
        acceptor_.async_accept(socket_, [this](boost::system::error_code error){
            if (!error)
            {
                try
                {
                    std::make_shared<T>(std::move(this->socket_))->start();
                }
                catch (boost::system::system_error const& err)
                {
                    spdlog::error("network", "Failed to retrieve client's remote address {}", err.what());
                }
            }
            
            if (!closed_)
                this->AsyncAccept<T>();
        });
    }


    template<AcceptCallback acceptCallback>
    void AsyncAcceptWithCallback()
    {
        boost::asio::ip::tcp::socket * sock;
        uint32_t threadIndex;

        std::tie(sock, threadIndex) = socketFactory_();

        acceptor_.async_accept(*sock, [this, sock, threadIndex](boost::system::error_code error){
            if (!error) 
            {
                try
                {
                    sock->non_blocking(true);

                    acceptCallback(std::move(*sock), threadIndex);
                }
                catch (boost::system::system_error const& err)
                {
                    spdlog::error("network Failed to initialize client's socket {}", err.what());
                }
            }

            if (!closed_) 
                this->AsyncAcceptWithCallback<acceptCallback>();
        });
    }


    bool bind()
    {
        
        boost::system::error_code errorCode;
        if (acceptor_.open(endpoint_.protocol(), errorCode)) 
        {
            spdlog::error("AsyncAcceptor::bind network Failed to open acceptor {}", errorCode.message());
            return false;
        }

        if (acceptor_.set_option(boost::asio::socket_base::reuse_address(true), errorCode)) 
        {
            spdlog::error("AsyncAcceptor::bind set_option Failed to reuse_address {}", errorCode.message());
            return false;
        }

        if (acceptor_.bind(endpoint_, errorCode)) 
        {
            spdlog::error("AsyncAcceptor::bind Could not bind to {}:{} {}", endpoint_.address().to_string(),
             endpoint_.port(), errorCode.message());

            return false;
        }


        if (acceptor_.listen(4096, errorCode)) 
        {
            spdlog::error("AsyncAcceptor::bind Failed to start listening on {}:{} {}", endpoint_.address().to_string(), endpoint_.port(), errorCode.message());

            return false;
        }
        

        return true;
    }

    void close()
    {
        if (closed_.exchange(true)) 
        {
            return;
        }

        acceptor_.close();
    }


    void setSockFactory(std::function<std::pair<boost::asio::ip::tcp::socket*, uint32_t>()> factory)
    {
        socketFactory_ = factory;
    }

private:
    std::pair<boost::asio::ip::tcp::socket*, uint32_t> defeaultSocketFactory()
    {
        return std::make_pair(&socket_, 0);
    }

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::endpoint endpoint_;
    boost::asio::ip::tcp::socket socket_;
    std::atomic<bool> closed_;

    std::function<std::pair<boost::asio::ip::tcp::socket*, uint32_t>()> socketFactory_;
};


}