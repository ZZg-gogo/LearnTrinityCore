#pragma once


#include "MySQLConnection.h"
#include <atomic>
#include <memory>


namespace DATABASE
{


class MysqlConnection;
class DatabaseWorker
{
public:
    using ptr = std::shared_ptr<DatabaseWorker>;

public:
    DatabaseWorker(MysqlConnection* conn, THREADING::ProducerConsumerQueue<SQLOperation*>* sqlQueue);
    ~DatabaseWorker();

private:
    void WorkerThread();

private:
    std::atomic<bool> cancel_;
    MysqlConnection* conn_;
    THREADING::ProducerConsumerQueue<SQLOperation*>* sqlQueue_; // sql队列
    std::thread workerThread_;

};    



}// namespace DATABASE