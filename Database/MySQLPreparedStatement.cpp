#include "MySQLPreparedStatement.h"
#include "MySQLHacks.h"
#include "PreparedStatement.h"
#include <cstdint>
#include <mysql/mysql.h>
#include <memory.h>


namespace DATABASE 
{

template<typename T>
struct MySQLType { };

template<> struct MySQLType<uint8_t> : std::integral_constant<enum_field_types, MYSQL_TYPE_TINY> { };
template<> struct MySQLType<uint16_t> : std::integral_constant<enum_field_types, MYSQL_TYPE_SHORT> { };
template<> struct MySQLType<uint32_t> : std::integral_constant<enum_field_types, MYSQL_TYPE_LONG> { };
template<> struct MySQLType<uint64_t> : std::integral_constant<enum_field_types, MYSQL_TYPE_LONGLONG> { };
template<> struct MySQLType<int8_t> : std::integral_constant<enum_field_types, MYSQL_TYPE_TINY> { };
template<> struct MySQLType<int16_t> : std::integral_constant<enum_field_types, MYSQL_TYPE_SHORT> { };
template<> struct MySQLType<int32_t> : std::integral_constant<enum_field_types, MYSQL_TYPE_LONG> { };
template<> struct MySQLType<int64_t> : std::integral_constant<enum_field_types, MYSQL_TYPE_LONGLONG> { };
template<> struct MySQLType<float> : std::integral_constant<enum_field_types, MYSQL_TYPE_FLOAT> { };
template<> struct MySQLType<double> : std::integral_constant<enum_field_types, MYSQL_TYPE_DOUBLE> { };



void MySQLPreparedStatement::setParameter(uint8_t index, std::nullptr_t)
{
    paramsSet_[index] = true;
    MYSQL_BIND* param = &bind_[index];
    param->buffer_type = MYSQL_TYPE_NULL;
    delete[] static_cast<char*>(param->buffer);
    param->buffer = nullptr;
    param->buffer_length = 0;
    param->is_null_value = 1;
    delete param->length;
    param->length = nullptr;
}

void MySQLPreparedStatement::setParameter(uint8_t index, bool value)
{
    setParameter(index, uint8_t(value ? 1 : 0));
}

template<typename T>
void MySQLPreparedStatement::setParameter(uint8_t index, T value)
{
    paramsSet_[index] = true;
    MYSQL_BIND* param = &bind_[index];
    uint32_t len = uint32_t(sizeof(T));
    param->buffer_type = MySQLType<T>::value;
    delete[] static_cast<char*>(param->buffer);
    param->buffer = new char[len];
    param->buffer_length = 0;
    param->is_null_value = 0;
    param->length = nullptr; 
    param->is_unsigned = std::is_unsigned_v<T>;

    memcpy(param->buffer, &value, len);
}

void MySQLPreparedStatement::setParameter(uint8_t index, std::string const& value)
{
    paramsSet_[index] = true;
    MYSQL_BIND* param = &bind_[index];
    uint32_t len = uint32_t(value.size());
    param->buffer_type = MYSQL_TYPE_VAR_STRING; //指定要发送到服务器值的变量类型
    delete [] static_cast<char*>(param->buffer);
    param->buffer = new char[len];
    param->buffer_length = len; //数据缓冲区的长度
    param->is_null_value = 0;
    delete param->length;
    param->length = new unsigned long(len); //存放的实际字节数

    //存储语句变量的数据值
    memcpy(param->buffer, value.c_str(), len);
}

void MySQLPreparedStatement::setParameter(uint8_t index, std::vector<uint8_t> const& value)
{
    paramsSet_[index] = true;
    MYSQL_BIND* param = &bind_[index];
    uint32_t len = uint32_t(value.size());
    param->buffer_type = MYSQL_TYPE_BLOB;
    delete [] static_cast<char*>(param->buffer);
    param->buffer = new char[len];
    param->buffer_length = len;
    param->is_null_value = 0;
    delete param->length;
    param->length = new unsigned long(len);

    memcpy(param->buffer, value.data(), len);
}


MySQLPreparedStatement::MySQLPreparedStatement(MySQLStmt * stmt, const std::string& sql) :
    base_(nullptr),
    stmt_(stmt),
    paramCount_(0),
    paramsSet_(),
    queryString_(sql)
{
    paramCount_ = mysql_stmt_param_count(stmt_);
    paramsSet_.assign(paramCount_, false);
    bind_ = new MySQLBind[paramCount_];
    memset(bind_, 0, sizeof(MySQLBind) * paramCount_);

    MySQLBool tmp = MySQLBool(1);
    mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &tmp);
}

MySQLPreparedStatement::~MySQLPreparedStatement()
{
    clearParameters();
    //如果绑定已经完成的话 释放由mysql客户端动态分配的内存
    if (stmt_->bind_result_done)
    {
        delete[] stmt_->bind->length;
        delete[] stmt_->bind->is_null;
    }
    //释放预处理对象
    mysql_stmt_close(stmt_);
    delete[] bind_;
}

void MySQLPreparedStatement::clearParameters()
{
    for (int i = 0; i < paramCount_; ++i) 
    {
        delete bind_[i].length;
        bind_[i].length = nullptr;
        delete[] (char*) bind_[i].buffer;
        bind_[i].buffer = nullptr;
        paramsSet_[i] = false;
    }
}


void MySQLPreparedStatement::bindPrameters(PreparedStatementBase *  base)
{
    base_ = base;
    uint8_t pos = 0;

    for (const PreparedStatementData & data : base_->GetParameters())
    {
        std::visit([&](auto&& param)
        {
            setParameter(pos, param);
        }, data.data);
        ++pos;
    }
}

}