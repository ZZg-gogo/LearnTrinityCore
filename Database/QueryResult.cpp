#include "QueryResult.h"
#include <cstddef>
#include <mysql/mysql.h>
#include <cstdint>
#include <spdlog/spdlog.h>
#include "DatabaseEnvFwd.h"
#include "Field.h"
#include "MySQLHacks.h"

namespace DATABASE
{

static uint32_t SizeForType(MYSQL_FIELD* field)
{
    switch (field->type)
    {
        case MYSQL_TYPE_NULL:
            return 0;
        case MYSQL_TYPE_TINY:
            return 1;
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_SHORT:
            return 2;
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_FLOAT:
            return 4;
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_BIT:
            return 8;

        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATETIME:
            return sizeof(MYSQL_TIME);

        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
            return field->max_length + 1;

        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            return 64;

        case MYSQL_TYPE_GEOMETRY:
            /*
            Following types are not sent over the wire:
            MYSQL_TYPE_ENUM:
            MYSQL_TYPE_SET:
            */
        default:
            return 0;
    }
}

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
        InitializeDatabaseFieldMetadata(&fieldMetadata_[i], &fields_[i], i);
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


PreparedResultSet::PreparedResultSet(MySQLStmt* stmt,
    MySQLResult* result,
    uint64_t rowCount,
    uint32_t fieldCount) :
    rowCount_(rowCount),
    rowPos_(0),
    fieldCount_(fieldCount),
    rows_(),
    fieldMetadata_(),
    stmt_(stmt),
    result_(result),
    bind_(nullptr)
{
    if (!result_) 
        return;

    if (stmt_->bind_result_done) 
    {
        delete [] stmt_->bind->length;
        delete [] stmt_->bind->is_null; 
    }

    bind_ = new MySQLBind[fieldCount_];
    MySQLBool* isNull = new MySQLBool[fieldCount_];
    unsigned long* length = new unsigned long[fieldCount_];

    memset(isNull, 0, sizeof(MySQLBool) * fieldCount_);
    memset(bind_, 0, sizeof(MySQLBind) * fieldCount_);
    memset(length, 0, sizeof(unsigned long) * fieldCount_);

    //缓存查询的完整结果
    if (mysql_stmt_store_result(stmt_)) 
    {
        spdlog::error("PreparedResultSet::PreparedResultSet mysql_stmt_store_result error");
        delete [] isNull;
        delete [] length;
        delete [] bind_;
        return;
    }

    rowCount_ = mysql_stmt_num_rows(stmt_);


    MySQLField* field = reinterpret_cast<MySQLField*>(mysql_fetch_fields(result_));
    fieldMetadata_.resize(fieldCount_);

    //计算出一行需要的空间
    std::size_t rowSize = 0;

    for (int i = 0; i < fieldCount_; ++i)
    {
        uint32_t size = SizeForType(&field[i]);
        rowSize += size;

        InitializeDatabaseFieldMetadata(&fieldMetadata_[i], &field[i], i , true);


        //绑定数据类型 可以用 mysql_fetch_fields 获取字段信息
        bind_[i].buffer_type = field[i].type;
        bind_[i].buffer_length = size;
        bind_[i].length = &length[i];
        bind_[i].is_null = &isNull[i];
        bind_[i].error = nullptr;
        bind_[i].is_unsigned = field[i].flags & UNSIGNED_FLAG;
    }

    //分配出所有查询结果的内存 行空间*总行数
    char* dataBuffer = new char[rowSize * rowCount_];
    //初始化绑定第一行的缓冲区
    for (uint32_t i = 0, offset = 0; i < fieldCount_; ++i)
    {
        bind_[i].buffer = dataBuffer + offset;
        offset += bind_[i].buffer_length;
    }

    //绑定结果集对象 指定缓冲区位置和相关信息
    if (mysql_stmt_bind_result(stmt_, bind_)) 
    {
        spdlog::error("PreparedResultSet::PreparedResultSet mysql_stmt_bind_result error");
        mysql_stmt_free_result(stmt_);

        cleanUp();
        delete [] isNull;
        delete [] length;
        return ;
    }

    rows_.resize(std::size_t(rowCount_) * fieldCount_);

    int count = 0;
    while (_nextRow()) 
    {
        for (int fIndex = 0; fIndex < fieldCount_; ++fIndex ) 
        {
            rows_[std::size_t(rowPos_)*fieldCount_+fIndex].setMetadata(&fieldMetadata_[fIndex]);

            //缓冲区的长度
            unsigned long buffer_length = bind_[fIndex].buffer_length;
            //实际拿到的字节长度
            unsigned long fetched_length = *bind_[fIndex].length;

            //如果查询出来的结果不是空的话
            if (!* bind_[fIndex].is_null) 
            {
                void * buffer = stmt_->bind[fIndex].buffer;

                switch (bind_[fIndex].buffer_type)
                {
                    case MYSQL_TYPE_TINY_BLOB:
                    case MYSQL_TYPE_MEDIUM_BLOB:
                    case MYSQL_TYPE_LONG_BLOB:
                    case MYSQL_TYPE_BLOB:
                    case MYSQL_TYPE_STRING:
                        break;
                    case MYSQL_TYPE_VAR_STRING:
                        if (fetched_length < buffer_length)
                            *((char*)buffer + fetched_length) = '\0';
                        break;
                    default:
                        break;
                }

                rows_[std::size_t(rowPos_)*fieldCount_+fIndex].setValue((const char*)buffer, fetched_length, true);
                
                //绑定缓冲区进行偏移
                stmt_->bind[fIndex].buffer = (char*)buffer + rowSize;
            }
            else
            {
                rows_[std::size_t(rowPos_)*fieldCount_+fIndex].setValue(nullptr, *bind_[fIndex].length);
            }

            
        }

        ++rowPos_;
    }

    rowPos_ = 0;
    //释放查询结果
    mysql_stmt_free_result(stmt_);
}   


PreparedResultSet::~PreparedResultSet()
{
    cleanUp();
}


void PreparedResultSet::cleanUp()
{
    if (result_) 
    {
        mysql_free_result(result_);
        result_ = nullptr;
    }

    if (bind_) 
    {
        delete[](char*)bind_->buffer;
        delete[] bind_;
        bind_ = nullptr;
    }
}

bool PreparedResultSet::nextRow()
{
    if (++rowPos_ >= rowCount_)
        return false;

    return true;
}

bool PreparedResultSet::_nextRow()
{
    if (rowPos_ >= rowCount_)
        return false;

    //获取到下一行的数据
    int retval = mysql_stmt_fetch(stmt_);
    return retval == 0 || retval == MYSQL_DATA_TRUNCATED;
}

Field* PreparedResultSet::Fetch() const
{
    return  const_cast<Field*>(&rows_[std::size_t(rowPos_) * fieldCount_]);
}
const Field & PreparedResultSet::operator[](std::size_t index) const
{
    return  rows_[std::size_t(rowPos_) * fieldCount_ + index];
}

const QueryResultFieldMetadata & PreparedResultSet::getFieldMetadata(std::size_t index) const
{
    return fieldMetadata_[index];
}


}//end namespace DATABASE