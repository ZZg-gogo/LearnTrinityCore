#include "PreparedStatement.h"
#include "DatabaseEnvFwd.h"
#include "QueryResult.h"
#include "MySQLConnection.h"
#include "QueryResult.h"


namespace DATABASE 
{

PreparedStatementBase::PreparedStatementBase(uint32_t index, uint8_t capacity):
    index_(index),
    statemenData_(capacity)
{

}
PreparedStatementBase::~PreparedStatementBase()
{

}


void PreparedStatementBase::setNull(uint8_t index)
{
    statemenData_[index].data = nullptr;
}
void PreparedStatementBase::setBool(uint8_t index, bool value)
{
    statemenData_[index].data = value;
}

void PreparedStatementBase::setUInt8(uint8_t index, uint8_t value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setUInt16(uint8_t index, uint16_t value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setUInt32(uint8_t index, uint32_t value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setUInt64(uint8_t index, uint64_t value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setInt8(uint8_t index, int8_t value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setInt16(uint8_t index, int16_t value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setInt32(uint8_t index, int32_t value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setInt64(uint8_t index, int64_t value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setFloat(uint8_t index, float value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setDouble(uint8_t index, double value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setString(uint8_t index, std::string const& value)
{
    statemenData_[index].data = value;
}
void PreparedStatementBase::setBinary(uint8_t index, std::vector<uint8_t> const& value)
{
    statemenData_[index].data = value;
}    

PreparedStatementTask::PreparedStatementTask(PreparedStatementBase* stmt, bool async) :
    stmt_(stmt)
{
    hasResult_ = async;
    if (async) 
    {
        result_ = new PreparedQueryResultPromise();
    }
}

PreparedStatementTask::~PreparedStatementTask()
{
    delete stmt_;

    if (hasResult_ && result_) 
    {
        delete result_;
        result_ = nullptr;
    }
}

bool PreparedStatementTask::execute()
{

    if (hasResult_) 
    {
        PreparedResultSet* res = conn_->Query(stmt_);
        if (!res || !res->GetRowCount()) 
        {
            delete res;
            result_->set_value(PreparedQueryResult(nullptr));
            return false;
        }

        result_->set_value(PreparedQueryResult(res));
        return true;
    }

    return conn_->Execute(stmt_);
}

}