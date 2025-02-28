#include "AdhocStatement.h"
#include "DatabaseEnvFwd.h"
#include <cstdlib>
#include "MySQLConnection.h"
#include "QueryResult.h"

namespace DATABASE
{

BasicStatementTask::BasicStatementTask(const char * sql, bool async) :
    sql_(sql),
    hasResult_(async)
{
    if (async) 
    {
        result_ = new QueryResultPromise();
    }
}

BasicStatementTask::~BasicStatementTask()
{
    free((void*)sql_);
    if (hasResult_ && result_)
    {
        delete result_;
        result_ = nullptr;
    }
}

bool BasicStatementTask::execute()
{
    if (hasResult_) 
    {
        ResultSet* result = conn_->Query(sql_);

        if (!result || !result->GetRowCount() || !result->nextRow())
        {
            delete result;
            result_->set_value(QueryResult(nullptr));
            return false;
        }
        else
        {
            result_->set_value(QueryResult(result));
            return true;
        }
    }

    return conn_->Execute(sql_);
}


}