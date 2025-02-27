#pragma once


#include <cstddef>
#include <functional>
#include <memory>
#include <vector>
#include "DatabaseEnvFwd.h"
#include "SQLOperation.h"
#include "PreparedStatement.h"

namespace DATABASE
{

class TransactionBase
{
public:
    using ptr = std::shared_ptr<TransactionBase>;
    friend class TransactionTask;
    friend class MysqlConnection;

public:
    TransactionBase() : cleanUp_(false)
    {}

    virtual ~TransactionBase() {cleanUp();}

    void append(const char * sql);

    std::size_t getSize() const {return sqls_.size();}

protected:
    void appendPreparedStatement(PreparedStatementBase* statement);
    void cleanUp();
    std::vector<SQLElementData> sqls_;
private:
    bool cleanUp_;
};


template<typename T>
class Transaction : public TransactionBase
{
public:
    using TransactionBase::append;
    void Append(PreparedStatement<T>* statement)
    {
        this->appendPreparedStatement(statement);
    }
};








}// namespace DATABASE