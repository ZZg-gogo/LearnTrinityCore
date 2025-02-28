#pragma once


#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "MySQLConnection.h"
#include "SQLOperation.h"
#include "QueryCallback.h"

namespace THREADING
{
template<typename T>
class ProducerConsumerQueue;
    
}

namespace DATABASE
{



class SQLOperation;
struct MysqlConnectionInfo;

template<typename T>
class DatabaseWorkerPool
{
public:
    using ptr = std::shared_ptr<DatabaseWorkerPool<T>>;

    enum InternalIndex
    {
        IDX_ASYNC,  //异步连接
        IDX_SYNC,   //同步连接
        IDX_SIZE    //支持的连接类型数量
    };

public:
    DatabaseWorkerPool();
    ~DatabaseWorkerPool();

    /**
    * @brief 设置连接相关信息
    * @param connInfo 连接信息字符串
    * @param asyncThreadCount 异步连接线程数量
    * @param syncThreadCount 同步连接线程数量
    */
    void setConnectionInfo(const std::string& connInfo,
        uint8_t asyncThreadCount,
        uint8_t syncThreadCount);


    uint32_t open();

    void close();

    //! Prepares all prepared statements
    bool prepareStatements();
    
    void keepAlive();
    size_t QueueSize() const { return queue_->size(); }

    //异步执行一条sql语句
    void Execute(const char * sql);
    void Execute(PreparedStatement<T>* stmt);

    //在调用线程直接执行sql语句
    void DirectExecute(const char * sql);
    void DirectExecute(PreparedStatement<T>* stmt);

    //同步查询
    QueryResult Query(char const* sql, T* connection = nullptr);
    PreparedQueryResult Query(PreparedStatement<T>* stmt);

    //异步查询
    QueryCallback AsyncQuery(char const* sql);
    QueryCallback AsyncQuery(PreparedStatement<T>* stmt);
private:
    uint32_t openConnections(uint8_t type, uint8_t count);

    void enterQueue(SQLOperation* sqlOp);

    /**
    * @brief 获取一个空闲的连接
    * @return 空闲的连接
    * @details 会一直等待 直到拿到空闲的连接
    */
    T * getFreeConnection();

private:
    //所有异步sql操作放到队列里面
    std::unique_ptr<THREADING::ProducerConsumerQueue<SQLOperation*>> queue_;
    //异步连接+同步连接
    std::array<std::vector<std::unique_ptr<T>>, IDX_SIZE> connections_;
    //连接信息
    std::unique_ptr<MysqlConnectionInfo> connectionInfo_;
    std::vector<uint8_t> preparedStatementSize_;
    uint8_t asyncThreadCount_;//异步操作的线程数量
    uint8_t syncThreadCount_;//同步操作的线程数量
};

}