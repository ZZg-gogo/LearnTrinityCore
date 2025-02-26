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
    uint64_t rowCount_;     //查询结果共有多少行
    uint32_t fieldCount_;   //一行有多少列
    Field * currentRow_;    //当前的字段数据
private:
    void cleanUp();
    MySQLResult * result_;  //存储查询结果集
    MySQLField * fields_;   //存储字段结构信息
};


class PreparedResultSet
{
public:
    using ptr = std::shared_ptr<PreparedResultSet>;
public:
    PreparedResultSet(MySQLStmt* stmt,
        MySQLResult* result,
        uint64_t rowCount,
        uint32_t fieldCount);

    ~PreparedResultSet();

    bool nextRow();

    uint64_t GetRowCount() const { return rowCount_; }
    uint64_t GetFieldCount() const { return fieldCount_; }
    
    Field* Fetch() const;
    Field const& operator[](std::size_t index) const;
    QueryResultFieldMetadata const& getFieldMetadata(std::size_t index) const;
    
protected:
    uint64_t rowCount_; //多少行
    uint64_t rowPos_;   //当前指向查询结果的第几行
    uint32_t fieldCount_;//一行有多少列
    std::vector<Field> rows_;//存储所有的行数据
    std::vector<QueryResultFieldMetadata> fieldMetadata_;
private:
    
    MySQLStmt *stmt_;
    MySQLResult * result_;
    MySQLBind* bind_;

    void cleanUp();

    bool _nextRow();

};



}