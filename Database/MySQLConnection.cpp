#include "MySQLConnection.h"
#include "../Utilities/Util.h"
#include <cstdint>
#include <mysql/errmsg.h>
#include <mysql/mysql.h>
#include <spdlog/spdlog.h>

namespace DATABASE 
{

MysqlConnectionInfo::MysqlConnectionInfo(const std::string& info)
{
    auto result = UTIL::stringSplit(info, ';');
    if (result.size() < 5) 
    {
        spdlog::error("MysqlConnectionInfo info error {}", info);
        return;
    }

    host = result[0];
    port = result[1];
    user = result[2];
    password = result[3];
    database = result[4];

    if (result.size() == 6) 
    {
        ssl = result[5];
    }
    
}


MysqlConnection::MysqlConnection(MysqlConnectionInfo& info) :
    reconnecting_(false),
    prepareError_(false),
    mysqlHandler_(nullptr),
    connectionInfo_(info),
    connectionFlags_(CONNECTION_SYNCH)
{

}

MysqlConnection::~MysqlConnection()
{
    close();
}


uint32_t MysqlConnection::open()
{
    MYSQL * mysqlInit;
    mysqlInit = mysql_init(nullptr);
    if (!mysqlInit)
    {
        spdlog::error("mysql_init open failed {}", connectionInfo_.database);
        return CR_UNKNOWN_ERROR;
    }

    int port;
    char const* unixSocket;
    //在连接之前调用 设置mysql客户端的字符集
    mysql_options(mysqlInit, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    //根据需要选择是 使用TCP/IP协议还是本地套接字协议
    if (connectionInfo_.host == ".") 
    {
        unsigned int opt = MYSQL_PROTOCOL_SOCKET;
        mysql_options(mysqlInit, MYSQL_OPT_PROTOCOL, &opt);
        connectionInfo_.host = "localhost";
        port = 0;
        unixSocket = connectionInfo_.port.c_str();
    }
    else 
    {
        port = std::stoi(connectionInfo_.port);
        unixSocket = nullptr;
    }

    //连接到数据库
    mysqlHandler_ = reinterpret_cast<MySQLHandle*>(mysql_real_connect(mysqlInit, 
        connectionInfo_.host.c_str(), 
        connectionInfo_.user.c_str(),
        connectionInfo_.password.c_str(),
        connectionInfo_.database.c_str(),
        port, unixSocket, 0));

    //连接成功的话
    if (mysqlHandler_) 
    {
        spdlog::info("mysql_real_connect open success {}:{}", connectionInfo_.host, connectionInfo_.database);
        //设置当前连接的默认字符集
        mysql_set_character_set(mysqlHandler_, "utf8mb4");
        return 0;
    }
    else
    {
        spdlog::error("mysql_real_connect open failed {}:{}", connectionInfo_.host, mysql_error(mysqlInit));
        //返回mysql的错误码
        uint32_t errorCode = mysql_errno(mysqlInit);
        mysql_close(mysqlInit);
        return errorCode;
    }

    
}

void MysqlConnection::close()
{
    if (mysqlHandler_) 
    {
        mysql_close(mysqlHandler_);
        mysqlHandler_ = nullptr;
    }
    
}

}// namespace DATABASE 