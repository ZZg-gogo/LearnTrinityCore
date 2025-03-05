#include "PingSocketMgr.h"
#include <boost/asio/io_context.hpp>
#include <cstdlib>
#include <spdlog/spdlog.h>


int main(int argc, char* argv[])
{

    if (argc != 3)
    {
        spdlog::error("Usage: PingPongServer <port> <threads>");
        return 1;
    }



    auto mgr = PingSocketMgr::getInstance();

    boost::asio::io_context ioc;
    mgr->startNetwork(ioc, "0.0.0.0", std::atoi(argv[1]), std::atoi(argv[2]));

    ioc.run();
    

    return 0;
}