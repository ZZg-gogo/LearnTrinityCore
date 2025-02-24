#pragma once


#include <cstdint>
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

class ResultSet;

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
    CONNECTION_ASYNC = 0x1, //异步连接池
    CONNECTION_SYNCH = 0x2, //同步连接池
    CONNECTION_BOTH = CONNECTION_ASYNC | CONNECTION_SYNCH
};


/**
* @class MysqlConnection
* @brief mysql数据库连接对象, 支持异步连接和同步连接
*/
class MysqlConnection
{
public: 
    using ptr = std::shared_ptr<MysqlConnection>;

public:
    /**
    * @brief 构造函数
    * @param info 数据库连接信息 同步
    */
    MysqlConnection(MysqlConnectionInfo& info);
    virtual ~MysqlConnection();

public:
    /**
    * @brief 打开数据库连接
    * @return uint32_t 连接状态
    */
    virtual uint32_t open();

    /**
    * @brief 关闭数据库连接
    */
    void close();

    /**
    * @brief sync执行sql语句
    * @param sql 待执行的sql语句
    * @return bool 执行结果
    */
    bool Execute(const char * sql);

    /**
    * @brief 预处理sql语句
    */
    bool PrepareStatements();

    /**
    * @brief 查询sql语句
    * @param sql 待查询的sql语句
    * @return ResultSet * 查询结果
    */
    ResultSet * Query(const char * sql);

    /**
    * @brief 查询sql语句
    * @param sql 待查询的sql语句
    * @param pResult 查询结果
    * @param pFields 查询的字段
    * @param pRowCount 查询的行数
    * @param pFieldCount 查询的字段数
    */
    bool _Query(char const* sql, MySQLResult** pResult, MySQLField** pFields, uint64_t* pRowCount, uint32_t* pFieldCount);
private:
    /**
    * @brief 处理mysql错误
    * @param errNo 错误码
    * @param attempts 尝试次数
    * @return bool 错误处理结果
    */
    bool _HandleMySQLErrno(uint32_t errNo, uint8_t attempts = 5);

protected:
    /**
    * @brief 预处理sql语句
    */
    virtual void doPrepareStatements(); 
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