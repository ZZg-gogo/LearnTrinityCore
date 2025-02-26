#pragma once


#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace DATABASE
{


enum class DatabaseFieldTypes : uint8_t
{
    Null,
    UInt8,
    Int8,
    UInt16,
    Int16,
    UInt32,
    Int32,
    UInt64,
    Int64,
    Float,
    Double,
    Decimal,
    Date,
    Time,
    Binary  
};


struct QueryResultFieldMetadata
{
    char const* TableName = nullptr;
    char const* TableAlias = nullptr;
    char const* Name = nullptr;
    char const* Alias = nullptr;
    char const* TypeName = nullptr;
    uint32_t Index = 0;
    DatabaseFieldTypes Type = DatabaseFieldTypes::Null;
};



/**
    @class Field

    @brief Class used to access individual fields of database query result

    Guideline on field type matching:

    |   MySQL type           |  method to use                         |
    |------------------------|----------------------------------------|
    | TINYINT                | GetBool, GetInt8, GetUInt8             |
    | SMALLINT               | GetInt16, GetUInt16                    |
    | MEDIUMINT, INT         | GetInt32, GetUInt32                    |
    | BIGINT                 | GetInt64, GetUInt64                    |
    | FLOAT                  | GetFloat                               |
    | DOUBLE, DECIMAL        | GetDouble                              |
    | CHAR, VARCHAR,         | GetCString, GetString                  |
    | TINYTEXT, MEDIUMTEXT,  | GetCString, GetString                  |
    | TEXT, LONGTEXT         | GetCString, GetString                  |
    | TINYBLOB, MEDIUMBLOB,  | GetBinary, GetString                   |
    | BLOB, LONGBLOB         | GetBinary, GetString                   |
    | BINARY, VARBINARY      | GetBinary                              |

    Return types of aggregate functions:

    | Function |       Type        |
    |----------|-------------------|
    | MIN, MAX | Same as the field |
    | SUM, AVG | DECIMAL           |
    | COUNT    | BIGINT            |
*/

class Field
{
public:
    friend class ResultSet;
    friend class PreparedResultSet;

    using ptr = std::shared_ptr<Field>;

public:
    Field();
    ~Field() = default;

    bool getBool() const
    {
        return getUint8() != 0;
    }

    uint8_t getUint8() const;
    int8_t getInt8() const;

    uint16_t getUint16() const;
    int16_t getInt16() const;

    uint32_t getUint32() const;
    int32_t getInt32() const;

    uint64_t getUint64() const;
    int64_t getInt64() const;

    float getFloat() const;
    double getDouble() const;

    const char* getCString() const;
    std::string getString() const;

    std::vector<uint8_t> getBinary() const;

    template <size_t S>
    std::array<uint8_t, S> getBinary() const
    {
        std::array<uint8_t, S> buf;
        getBinarySizeChecked(buf.data(), S);
        return buf;
    }

    bool IsNull() const
    {
        return value_ == nullptr;
    }
private:
    const char * value_;
    uint32_t length_;
    bool isPrepared_; 
    const QueryResultFieldMetadata * meta_;
  
    void setValue(const char* newValue, uint32_t newLength, bool isPrepared = false);

    void getBinarySizeChecked(uint8_t* buf, std::size_t size) const;

    void setMetadata(const QueryResultFieldMetadata * meta);
};


}// end namespace DATABASE