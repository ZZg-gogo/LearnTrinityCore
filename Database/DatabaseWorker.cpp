#include "DatabaseWorker.h"
#include "SQLOperation.h"
#include <fcntl.h>
#include <thread>


namespace DATABASE 
{

DatabaseWorker::DatabaseWorker(MysqlConnection* conn, THREADING::ProducerConsumerQueue<SQLOperation*>* sqlQueue):
    cancel_(false),
    conn_(conn),
    sqlQueue_(sqlQueue)
{
    workerThread_ = std::thread(&DatabaseWorker::WorkerThread, this);
}


DatabaseWorker::~DatabaseWorker()
{
    cancel_ = true;
    sqlQueue_->stop();
    workerThread_.join();
}

void DatabaseWorker::WorkerThread()
{
    if (!sqlQueue_) 
    {
        return;
    }


    while (true) 
    {
        SQLOperation *op = nullptr;

        if (cancel_ || sqlQueue_->get(op))
        {

        }

        if (op) 
        {
            op->setConnection(conn_);
            op->call();
        }

        op = nullptr;
    }
}


}