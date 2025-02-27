#pragma once


#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <sys/types.h>

#include "MySQLHacks.h"
#include "PreparedStatement.h"
#include "../Threading/ProducerConsumerQueue.h"

/**
* @file MySQLConnection.h
* @brief Myssql数据库连接
*/



namespace  DATABASE
{

class DatabaseWorker;
class ResultSet;
class MySQLPreparedStatement;

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
    using PreparedStatementContainer = std::vector<std::unique_ptr<MySQLPreparedStatement>>;
public:
    /**
    * @brief 构造函数
    * @param info 数据库连接信息 同步
    */
    MysqlConnection(MysqlConnectionInfo& info);

    /**
    * @brief 构造函数
    * @param queue sql任务队列
    * @param info 数据库连接信息 异步
    */
    MysqlConnection(THREADING::ProducerConsumerQueue<SQLOperation*>* queue, MysqlConnectionInfo& connInfo);

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
    * @brief sync执行预处理语句
    * @param base 预处理语句
    * @return bool 执行结果
    */
    bool Execute(PreparedStatementBase * base);

    /**
    * @brief 新增预处理sql语句
    * @param index 预处理语句的索引
    * @param sql 待处理的sql语句
    * @param flags 连接标志
    */
    void PreparedStatement(uint32_t index, const std::string& sql, ConnectionFlags flags);



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
    * @brief 执行预处理sql语句
    * @param base 预处理语句
    * @return PreparedResultSet* 查询结果
    */
    PreparedResultSet* Query(PreparedStatementBase * base);


    /**
    * @brief 查询预处理sql语句
    * @param base 预处理语句
    * @param mysqlStmt mysql预处理语句
    * @param pResult 查询结果
    * @param pRowCount 查询的行数
    * @param pFieldCount 查询的字段数
    */
    bool _Query(PreparedStatementBase* base,
        MySQLPreparedStatement** mysqlStmt,
        MySQLResult** pResult,
        uint64_t* pRowCount,
        uint32_t* pFieldCount);


    /**
    * @brief 查询sql语句
    * @param sql 待查询的sql语句
    * @param pResult 查询结果
    * @param pFields 查询的字段
    * @param pRowCount 查询的行数
    * @param pFieldCount 查询的字段数
    */
    bool _Query(char const* sql, MySQLResult** pResult, MySQLField** pFields, uint64_t* pRowCount, uint32_t* pFieldCount);

    void ping();

    uint32_t getLastError() const;
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

    /**
    * @brief 获取预处理语句
    * @param index 预处理语句的索引
    * @return MySQLPreparedStatement* 预处理语句
    */
    MySQLPreparedStatement* GetPreparedStatement(uint32_t index);
    bool LockIfReady()
    {
        return mutex_.try_lock();
    }

    void Unlock()
    {
        mutex_.unlock();
    }
protected:
    bool reconnecting_;                        // 是否正在重连
    bool prepareError_;                        // 预处理语句是否有错误
    PreparedStatementContainer preparedStatements_; // 预处理语句容器
private:
    std::mutex mutex_;                          // 互斥锁
    MySQLHandle * mysqlHandler_;                //数据库的句柄
    MysqlConnectionInfo& connectionInfo_;       // 数据库连接信息
    ConnectionFlags connectionFlags_;           // 连接标志 
    THREADING::ProducerConsumerQueue<SQLOperation*>* sqlQueue_; // sql队列
    std::unique_ptr<DatabaseWorker> worker_;    // 数据库工作线程
};



}// namespace  DATABASE