#pragma once


#include <memory>

namespace DATABASE 
{
class PreparedStatementBase;


union SQLElementUnion
{
    PreparedStatementBase* stmt;
    const char * query;
};

enum SQLElementDataType
{
    SQL_ELEMENT_RAW,
    SQL_ELEMENT_PREPARED
};

struct SQLElementData
{
    SQLElementUnion element;
    SQLElementDataType type;
};

class MysqlConnection;


class SQLOperation
{
public: 
    using ptr = std::shared_ptr<SQLOperation>;
public:
    SQLOperation(): conn_(nullptr) { }
    virtual ~SQLOperation() { }

    virtual int call()
    {
        execute();
        return 0;
    }

    virtual bool execute() = 0;

    virtual void setConnection(MysqlConnection* con) { conn_ = con;}

private:
    MysqlConnection* conn_;
};

}