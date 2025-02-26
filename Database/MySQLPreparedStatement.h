#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "MySQLHacks.h"

namespace DATABASE 
{

class MysqlConnection;
class PreparedStatementBase;


class MySQLPreparedStatement
{
public: 
    using ptr = std::shared_ptr<MySQLPreparedStatement>;
    friend class MysqlConnection; 
    friend class PreparedStatementBase;
public:
    MySQLPreparedStatement(MySQLStmt * stmt, const std::string& sql);
    ~MySQLPreparedStatement();

    uint32_t getParamCount() const { return paramCount_; }
    //将参数绑定到stmt_上
    void bindPrameters(PreparedStatementBase *  base);

protected:
    void setParameter(uint8_t index, std::nullptr_t);
    void setParameter(uint8_t index, bool value);
    template<typename T>
    void setParameter(uint8_t index, T value);
    void setParameter(uint8_t index, std::string const& value);
    void setParameter(uint8_t index, std::vector<uint8_t> const& value);

    MySQLStmt* getSTMT() { return stmt_; }
    MySQLBind* getBind() { return bind_; }
    void clearParameters();
    const std::string& getQueryString() { return queryString_; }
protected:
    //将base_里面的参数 都绑定到bind_上
    PreparedStatementBase * base_;
private:
    MySQLStmt * stmt_;
    uint32_t paramCount_;
    std::vector<bool> paramsSet_;
    MySQLBind* bind_;
    const std::string queryString_;

};



}// end namespace DATABASE