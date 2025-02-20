#pragma once


#include <memory>
#include <mutex>
#include <string>
#include <sys/types.h>

#include "MySQLHacks.h"

/**
* @file MySQLConnection.h
* @brief Myssql数据库连接
*/



namespace  DATABASE
{

struct MysqlConnectionInfo
{
    explicit MysqlConnectionInfo(const std::string& info);

    std::string host;
    std::string port;
    std::string user;
    std::string password;
    std::string database;
    std::string ssl;
};


/**
* @enum ConnectionFlags
* @brief 连接表示
*/
// 定义一个枚举类型 ConnectionFlags，用于表示连接的标志
enum ConnectionFlags
{
    CONNECTION_ASYNC = 0x1,
    CONNECTION_SYNCH = 0x2,
    CONNECTION_BOTH = CONNECTION_ASYNC | CONNECTION_SYNCH
};



class MysqlConnection
{
public: 
    using ptr = std::shared_ptr<MysqlConnection>;

public:
    MysqlConnection(MysqlConnectionInfo& info);
    virtual ~MysqlConnection();

    virtual uint32_t open();
    void close();
private:


protected:
    bool reconnecting_;                        // 是否正在重连
    bool prepareError_;                        // 预处理语句是否有错误
private:
    std::mutex mutex_;                          // 互斥锁
    MySQLHandle * mysqlHandler_;                //数据库的句柄
    MysqlConnectionInfo& connectionInfo_;       // 数据库连接信息
    ConnectionFlags connectionFlags_;           // 连接标志 
    
};



}// namespace  DATABASE