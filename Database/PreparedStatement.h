#pragma once


#include <cstdint>
#include <memory>
#include <variant>
#include <string>
#include <vector>
#include "DatabaseEnvFwd.h"
#include "SQLOperation.h"

namespace DATABASE 
{

/**
* @struct PreparedStatementData
* @brief 保存预处理语句的某个单个数据
*/
struct PreparedStatementData
{
    std::variant<
    bool,
    uint8_t,
    uint16_t,
    uint32_t,
    uint64_t,
    int8_t,
    int16_t,
    int32_t,
    int64_t,
    float,
    double,
    std::string,
    std::vector<uint8_t>,
    std::nullptr_t
    > data;
};


/**
* @class PreparedStatementBase
* @brief 预处理语句基类
* @details 保存预处理语句的相关参数 保存在预处理容器中的索引
*/
class PreparedStatementBase
{
public:
    friend class PreparedStatementTask;
    using ptr = std::shared_ptr<PreparedStatementBase>;

public:

    /**
    * @brief 构造函数
    * @param index 预处理语句在预处理容器中的索引
    * @param capacity 预处理语句的参数个数
    */
    explicit PreparedStatementBase(uint32_t index, uint8_t capacity);
    virtual ~PreparedStatementBase();

    //设置index位置的值
    void setNull(uint8_t index);
    void setBool(uint8_t index, bool value);
    void setUInt8(uint8_t index, uint8_t value);
    void setUInt16(uint8_t index, uint16_t value);
    void setUInt32(uint8_t index, uint32_t value);
    void setUInt64(uint8_t index, uint64_t value);
    void setInt8(uint8_t index, int8_t value);
    void setInt16(uint8_t index, int16_t value);
    void setInt32(uint8_t index, int32_t value);
    void setInt64(uint8_t index, int64_t value);
    void setFloat(uint8_t index, float value);
    void setDouble(uint8_t index, double value);
    void setString(uint8_t index, std::string const& value);
    void setBinary(uint8_t index, std::vector<uint8_t> const& value);
    template <size_t Size>
    void setBinary(const uint8_t index, std::array<uint8_t, Size> const& value)
    {
        std::vector<uint8_t> vec(value.begin(), value.end());
        setBinary(index, vec);
    }


    uint32_t GetIndex() const { return index_; }
    std::vector<PreparedStatementData> const& GetParameters() const { return statemenData_; }
protected:
    uint32_t index_;    //在预处理容器中的索引(所有的预处理语句会被放在一个容器中)
    std::vector<PreparedStatementData> statemenData_;   //存放该预处理语句的数据

};



template<typename T>
class PreparedStatement : public PreparedStatementBase
{
public:
    explicit PreparedStatement(uint32_t index, uint8_t capacity) : PreparedStatementBase(index, capacity)
    {
    }
};


class PreparedStatementTask : public SQLOperation
{
    public:
        PreparedStatementTask(PreparedStatementBase* stmt, bool async = false);
        ~PreparedStatementTask();

        bool execute() override;    
        PreparedQueryResultFuture getFuture() { return result_->get_future(); }

    protected:
        PreparedStatementBase* stmt_;
        bool hasResult_;
        PreparedQueryResultPromise* result_;
};

}