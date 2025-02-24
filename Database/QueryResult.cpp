#include "QueryResult.h"
#include <cstdint>
#include <mysql/mysql.h>
#include <spdlog/spdlog.h>
#include "Field.h"

namespace DATABASE
{

static const char* FieldTypeToString(enum_field_types type, uint32_t flags)
{
    switch (type)
    {
        case MYSQL_TYPE_BIT:         return "BIT";
        case MYSQL_TYPE_BLOB:        return "BLOB";
        case MYSQL_TYPE_DATE:        return "DATE";
        case MYSQL_TYPE_DATETIME:    return "DATETIME";
        case MYSQL_TYPE_NEWDECIMAL:  return "NEWDECIMAL";
        case MYSQL_TYPE_DECIMAL:     return "DECIMAL";
        case MYSQL_TYPE_DOUBLE:      return "DOUBLE";
        case MYSQL_TYPE_ENUM:        return "ENUM";
        case MYSQL_TYPE_FLOAT:       return "FLOAT";
        case MYSQL_TYPE_GEOMETRY:    return "GEOMETRY";
        case MYSQL_TYPE_INT24:       return (flags & UNSIGNED_FLAG) ? "UNSIGNED INT24" : "INT24";
        case MYSQL_TYPE_LONG:        return (flags & UNSIGNED_FLAG) ? "UNSIGNED LONG" : "LONG";
        case MYSQL_TYPE_LONGLONG:    return (flags & UNSIGNED_FLAG) ? "UNSIGNED LONGLONG" : "LONGLONG";
        case MYSQL_TYPE_LONG_BLOB:   return "LONG_BLOB";
        case MYSQL_TYPE_MEDIUM_BLOB: return "MEDIUM_BLOB";
        case MYSQL_TYPE_NEWDATE:     return "NEWDATE";
        case MYSQL_TYPE_NULL:        return "NULL";
        case MYSQL_TYPE_SET:         return "SET";
        case MYSQL_TYPE_SHORT:       return (flags & UNSIGNED_FLAG) ? "UNSIGNED SHORT" : "SHORT";
        case MYSQL_TYPE_STRING:      return "STRING";
        case MYSQL_TYPE_TIME:        return "TIME";
        case MYSQL_TYPE_TIMESTAMP:   return "TIMESTAMP";
        case MYSQL_TYPE_TINY:        return (flags & UNSIGNED_FLAG) ? "UNSIGNED TINY" : "TINY";
        case MYSQL_TYPE_TINY_BLOB:   return "TINY_BLOB";
        case MYSQL_TYPE_VAR_STRING:  return "VAR_STRING";
        case MYSQL_TYPE_YEAR:        return "YEAR";
        default:                     return "-Unknown-";
    }
}

DatabaseFieldTypes MysqlTypeToFieldType(enum_field_types type, uint32_t flags)
{
    switch (type)
    {
        case MYSQL_TYPE_NULL:
            return DatabaseFieldTypes::Null;
        case MYSQL_TYPE_TINY:
            return (flags & UNSIGNED_FLAG) ? DatabaseFieldTypes::UInt8 : DatabaseFieldTypes::Int8;
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_SHORT:
            return (flags & UNSIGNED_FLAG) ? DatabaseFieldTypes::UInt16 : DatabaseFieldTypes::Int16;
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
            return (flags & UNSIGNED_FLAG) ? DatabaseFieldTypes::UInt32 : DatabaseFieldTypes::Int32;
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_BIT:
            return (flags & UNSIGNED_FLAG) ? DatabaseFieldTypes::UInt64 : DatabaseFieldTypes::Int64;
        case MYSQL_TYPE_FLOAT:
            return DatabaseFieldTypes::Float;
        case MYSQL_TYPE_DOUBLE:
            return DatabaseFieldTypes::Double;
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            return DatabaseFieldTypes::Decimal;
        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_DATETIME:
            return DatabaseFieldTypes::Date;
        case MYSQL_TYPE_TIME:
            return DatabaseFieldTypes::Time;
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
            return DatabaseFieldTypes::Binary;
        default:
            spdlog::info("MysqlTypeToFieldType sql MysqlTypeToFieldType(): invalid field type {}", (uint32_t)(type));
            break;
    }

    return DatabaseFieldTypes::Null;
}

void InitializeDatabaseFieldMetadata(QueryResultFieldMetadata* meta,
    MySQLField const* field,
    uint32_t fieldIndex,
    bool binaryProtocol = false)
{
    meta->TableName = field->org_table;
    meta->TableAlias = field->table;
    meta->Name = field->org_name;
    meta->Alias = field->name;
    meta->TypeName = FieldTypeToString(field->type, field->flags);
    meta->Index = fieldIndex;
    meta->Type = MysqlTypeToFieldType(field->type, field->flags);
}


/**
* @brief 构造函数
* @param result 查询结果集
* @param fields 字段信息
* @param rowCount 行数
* @param fieldCount 字段数
*/
ResultSet::ResultSet(MySQLResult * result, 
    MySQLField * fields,
    uint64_t rowCount,
    uint32_t fieldCount) :
    rowCount_(rowCount),
    fieldCount_(fieldCount),
    result_(result),
    fields_(fields)
{
    fieldMetadata_.resize(fieldCount);

    currentRow_ = new Field[fieldCount];

    for (int i = 0; i < fieldCount; ++i) 
    {
        InitializeDatabaseFieldMetadata(&fieldMetadata_[i], &fields[i], i);
        currentRow_[i].setMetadata(&fieldMetadata_[i]);
    }
}


ResultSet::~ResultSet()
{
    cleanUp();
}

/**
* @brief 获取下一行数据
* @return 获取是否成功
*/
bool ResultSet::nextRow()
{
    if (!result_) 
    {
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result_);
    if (!row) //没有下一行的数据了
    {
        cleanUp();
        return false;
    }


    unsigned long* lengths = mysql_fetch_lengths(result_);
    if (!lengths) //没有获取到每一列的数据的长度
    {
        cleanUp();
        return false;
    }

    for (int i = 0; i < fieldCount_; ++i)
    {
        currentRow_[i].setValue(row[i],lengths[i]);
    }

    return true;
}

const Field & ResultSet::operator[](std::size_t index) const
{
    return currentRow_[index];
}

const QueryResultFieldMetadata & ResultSet::GetFieldMetadata(std::size_t index) const
{
    return fieldMetadata_[index];
}


void ResultSet::cleanUp()
{
    if (result_) 
    {
        mysql_free_result(result_);
        result_ = nullptr;
    }


    if (currentRow_) 
    {
        delete[] currentRow_;
        currentRow_ = nullptr;
    }
}

}//end namespace DATABASE