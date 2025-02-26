#include "Field.h"
#include <memory.h>

namespace DATABASE {

Field::Field() : value_(nullptr), length_(0), meta_(nullptr) {}

uint8_t Field::getUint8() const {
  if (!value_)
    return 0;

  if(isPrepared_) 
  {
  
    return static_cast<uint8_t>(*(uint8_t*)value_);
  }
  return static_cast<uint8_t>(std::strtoul(value_, nullptr, 10));
}

int8_t Field::getInt8() const {
  if (!value_)
    return 0;

  if (isPrepared_) {
    
    return static_cast<int8_t>(*(int8_t*)value_);
  }

  return static_cast<int8_t>(std::strtol(value_, nullptr, 10));
}

uint16_t Field::getUint16() const {
  if (!value_)
    return 0;

  if(isPrepared_) 
  {
  
    return static_cast<uint16_t>(*(uint16_t*)value_);
  }
  return static_cast<uint16_t>(std::strtoul(value_, nullptr, 10));
}

int16_t Field::getInt16() const {
  if (!value_)
    return 0;

  if(isPrepared_) 
  {
  
    return static_cast<int16_t>(*(int16_t*)value_);
  }
  return static_cast<int16_t>(std::strtol(value_, nullptr, 10));
}

uint32_t Field::getUint32() const {
  if (!value_)
    return 0;

  if(isPrepared_) 
  {
  
    return static_cast<uint32_t>(*(uint32_t*)value_);
  }
  return static_cast<uint32_t>(std::strtoul(value_, nullptr, 10));
}

int32_t Field::getInt32() const {
  if (!value_)
    return 0;

  if(isPrepared_) 
  {
  
    return static_cast<int32_t>(*(int32_t*)value_);
  }
  return static_cast<int32_t>(std::strtol(value_, nullptr, 10));
}

uint64_t Field::getUint64() const {
  if (!value_)
    return 0;

  if(isPrepared_) 
  {
  
    return static_cast<uint64_t>(*(uint64_t*)value_);
  }
  return static_cast<uint64_t>(std::strtoull(value_, nullptr, 10));
}

int64_t Field::getInt64() const {
  if (!value_)
    return 0;

  if(isPrepared_) 
  {
  
    return static_cast<int64_t>(*(int64_t*)value_);
  }
  return static_cast<int64_t>(std::strtoll(value_, nullptr, 10));
}

float Field::getFloat() const {
  if (!value_)
    return 0.0f;

  if(isPrepared_) 
  {
  
    return static_cast<float>(*(float*)value_);
  }
  return std::strtof(value_, nullptr);
}

double Field::getDouble() const {
  if (!value_)
    return 0.0;

  if(isPrepared_) 
  {
  
    return static_cast<double>(*(double*)value_);
  }
  return std::strtod(value_, nullptr);
}

const char *Field::getCString() const { return value_ ? value_ : nullptr; }

std::string Field::getString() const {
  return value_ ? std::string(value_, length_) : "";
}

std::vector<uint8_t> Field::getBinary() const {
  std::vector<uint8_t> result;
  if (!value_ || !length_)
    return result;
  result.resize(length_);
  memcpy(result.data(), value_, length_);
  return result;
}

void Field::getBinarySizeChecked(uint8_t *buf, std::size_t size) const {
  if (value_ && size <= length_) {
    memcpy(buf, value_, size);
  }
}

void Field::setValue(const char *newValue, uint32_t newLength, bool isPrepared) {
  value_ = newValue;
  length_ = newLength;
  isPrepared_ = isPrepared;
}

void Field::setMetadata(const QueryResultFieldMetadata *meta) { meta_ = meta; }

} // namespace DATABASE