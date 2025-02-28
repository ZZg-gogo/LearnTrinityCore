#include "DatabaseWorkerPool.h"
#include "../Threading/ProducerConsumerQueue.h"
#include "DatabaseEnvFwd.h"
#include "DatabaseWorker.h"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <mysql/mysql.h>
#include <spdlog/spdlog.h>
#include "AdhocStatement.h"
#include "QueryCallback.h"
#include "QueryResult.h"

namespace DATABASE
{

class PingOperation : public SQLOperation
{
    //! Operation for idle delaythreads
    bool execute() override
    {
        conn_->ping();
        return true;
    }
};



template<typename T>
DatabaseWorkerPool<T>::DatabaseWorkerPool() :
    queue_(new THREADING::ProducerConsumerQueue<SQLOperation*>),
    asyncThreadCount_(0),
    syncThreadCount_(0)
{
    mysql_thread_safe();
}


template<typename T>
DatabaseWorkerPool<T>::~DatabaseWorkerPool()
{
    queue_->stop();
}


template<typename T>
void DatabaseWorkerPool<T>::enterQueue(SQLOperation* sqlOp)
{
    queue_->push(sqlOp);
}



template <typename T>
void DatabaseWorkerPool<T>::setConnectionInfo(const std::string& connInfo,
    uint8_t asyncThreadCount,
    uint8_t syncThreadCount)
{
    spdlog::info("DatabaseWorkerPool:setConnectionInfo info:{} asyncThreadCount:{} syncThreadCount:{}", 
        connInfo, asyncThreadCount, syncThreadCount);
    connectionInfo_ = std::make_unique<MysqlConnectionInfo>(connInfo);
    asyncThreadCount_ = asyncThreadCount;
    syncThreadCount_ = syncThreadCount;
}


template<typename T>
void DatabaseWorkerPool<T>::close()
{
    spdlog::info("DatabaseWorkerPool:close close");
    connections_[IDX_ASYNC].clear();
    connections_[IDX_SYNC].clear();
}


template<typename T>
uint32_t DatabaseWorkerPool<T>::open()
{
    uint32_t error = openConnections(IDX_ASYNC, asyncThreadCount_);

    if (error)
    {
        spdlog::error("DatabaseWorkerPool:open  fail");
        return error;
    }
        

    error = openConnections(IDX_SYNC, syncThreadCount_);

    if (!error)
        spdlog::info("DatabaseWorkerPool: open {} connections", asyncThreadCount_ + syncThreadCount_);
    else
        spdlog::error("DatabaseWorkerPool:open  fail");
    
    
    return error;
}


template<typename T>
uint32_t DatabaseWorkerPool<T>::openConnections(uint8_t type, uint8_t count)
{
    for (int i = 0; i < count; ++i) 
    {
        auto connection = [&]{
            switch (type) 
            {
            case IDX_SYNC:
                return std::make_unique<T>(connectionInfo_.get());
            case IDX_ASYNC:
                return std::make_unique<T>(queue_.get(), connectionInfo_.get());
            default:
                return nullptr;
            }
        }();

        if (uint32_t error = connection->open()) 
        {
            connections_[type].clear();
            return error;
        }
        else
        {
            connections_[type].push_back(std::move(connection()));
        }
    }

    spdlog::info("DatabaseWorkerPool: type {} Opened {} connections", type, count);
    return 0;

}




template<typename T>
T * DatabaseWorkerPool<T>::getFreeConnection()
{
    std::array<std::vector<std::unique_ptr<T>>, IDX_SIZE> connections_;

    auto &conns = connections_[IDX_SYNC];

    uint8_t i = 0;
    std::size_t nums = conns.size();
    T* connection = nullptr;
    for (;;) 
    {
        connection = conns[++i%nums].get();
        if (connection->LockIfReady()) 
        {
            break;
        }
    }

    return connection;
}

template<typename T>
void DatabaseWorkerPool<T>::keepAlive()
{
    for (auto& conn : connections_[IDX_SYNC]) 
    {
        if (conn->LockIfReady()) 
        {
            conn->ping();
            conn->Unlock();
        }
    }

    auto const count = connections_[IDX_ASYNC].size();
    for (uint8_t i = 0; i < count; ++i)
        enterQueue(new PingOperation); 

}

template<typename T>
void DatabaseWorkerPool<T>::Execute(const char * sql)
{
    BasicStatementTask * task = new BasicStatementTask(sql);
    enterQueue(task);
}


template<typename T>
void DatabaseWorkerPool<T>::Execute(PreparedStatement<T>* stmt)
{
    PreparedStatementTask * task = new PreparedStatementTask(stmt);
    enterQueue(task);
}



template<typename T>
void DatabaseWorkerPool<T>::DirectExecute(const char * sql)
{

    T* conn = getFreeConnection();

    conn->Execute(sql);
    conn->Unlock();;
}


template<typename T>
void DatabaseWorkerPool<T>::DirectExecute(PreparedStatement<T>* stmt)
{

    T* conn = getFreeConnection();

    conn->Execute(stmt);
    conn->Unlock();;
    delete stmt;
}
template<typename T>
QueryResult DatabaseWorkerPool<T>::Query(char const* sql, T* connection)
{
    if (!connection) 
    {   
        connection = getFreeConnection();
    }
    ResultSet* result = connection->Query(sql);
    connection->Unlock();
    if (!result || !result->GetRowCount() || !result->nextRow())
    {
        delete result;
        return QueryResult(nullptr);
    }

    return QueryResult(result);
}

template<typename T>
PreparedQueryResult DatabaseWorkerPool<T>::Query(PreparedStatement<T>* stmt)
{

    T * connection = getFreeConnection();
    
    PreparedResultSet* result = connection->Query(stmt);
    connection->Unlock();

    delete stmt;
    if (!result  || !result->GetRowCount())
    {
        delete result;
        return PreparedQueryResult(nullptr);
    }

    return PreparedQueryResult(result);
}

template<typename T>
QueryCallback DatabaseWorkerPool<T>::AsyncQuery(char const* sql)
{
    BasicStatementTask * task = new BasicStatementTask(sql, true);
    enterQueue(task);

    return QueryCallback(task->getFuture());
}


template<typename T>
QueryCallback DatabaseWorkerPool<T>::AsyncQuery(PreparedStatement<T>* stmt)
{
    PreparedStatementTask * task = new PreparedStatementTask(stmt, true);
    enterQueue(task);
    return QueryCallback(task->getFuture());
}


}