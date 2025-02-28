#pragma once

#include "SQLOperation.h"
#include "DatabaseEnvFwd.h"

namespace DATABASE
{

class BasicStatementTask : public SQLOperation
{
public:
    /**
    * @brief 构造函数
    * @param sql 普通的sql语句
    * @param async 是否异步执行
    * @details 如果是异步执行的话 就需要promise对象了
    */
    BasicStatementTask(const char * sql, bool async = false);
    ~BasicStatementTask();

    bool execute() override;
    QueryResultFuture getFuture() const { return result_->get_future(); }

private:
    const char * sql_;      //- Raw query to be executed
    bool hasResult_;
    QueryResultPromise* result_;
};



}