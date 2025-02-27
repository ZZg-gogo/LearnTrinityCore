#include "Transaction.h"
#include <string.h>

#include "SQLOperation.h"



namespace DATABASE 
{
void TransactionBase::appendPreparedStatement(PreparedStatementBase* statement)
{
    SQLElementData data;
    data.element.stmt = statement;
    data.type = SQL_ELEMENT_PREPARED;
    sqls_.push_back(data);
}

void TransactionBase::append(const char * sql)
{
    SQLElementData data;
    data.type = SQL_ELEMENT_RAW;
    data.element.query = strdup(sql);
    sqls_.push_back(data);
}

void TransactionBase::cleanUp()
{
    if (cleanUp_) 
    {
        return;
    }

    for ( auto& sql : sqls_) 
    {
        if (sql.type == SQL_ELEMENT_RAW) 
        {
            free((void*)(sql.element.query));
        }
        else if (sql.type == SQL_ELEMENT_PREPARED) 
        {
            delete sql.element.stmt;
        }
    }


    cleanUp_ = true;
}

}