#pragma once



#include <cstdint>
#include <memory>
#include <mysql/mysql.h>
#include <vector>
#include "Field.h"

#include "MySQLHacks.h"

namespace DATABASE 
{

/**
* @class ResultSet
* @brief 存储query结果集对象
*/
class ResultSet
{
public:
    using ptr =  std::shared_ptr<ResultSet>;

public:
    /**
    * @brief 构造函数
    * @param result 查询结果集
    * @param fields 字段信息
    * @param rowCount 行数
    * @param fieldCount 字段数
    */
    ResultSet(MySQLResult * result, 
            MySQLField * fields,
            uint64_t rowCount,
            uint32_t fieldCount);

    ~ResultSet();
    
    /**
    * @brief 获取下一行数据
    * @return 获取是否成功
    */
    bool nextRow();

    uint64_t GetRowCount() const { return rowCount_; }
    uint64_t GetFieldCount() const { return fieldCount_; }

    Field * fetch() const {return currentRow_;}
    
    Field const& operator[](std::size_t index) const;
    QueryResultFieldMetadata const& GetFieldMetadata(std::size_t index) const;
protected:
    std::vector<QueryResultFieldMetadata> fieldMetadata_;
    uint64_t rowCount_;
    uint32_t fieldCount_;
    Field * currentRow_;
private:
    void cleanUp();
    MySQLResult * result_;
    MySQLField * fields_;
};



}