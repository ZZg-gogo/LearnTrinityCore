#pragma once


/**
* @file Util.h
* @brief 工具类
*/

#include <vector>
#include <string>

namespace UTIL
{

/**
* @brief 字符串分割
* @param str 待分割字符串
* @param delimiter 分隔符
* @param keepEmpty 是否保留空字符串
* @return 分割后的字符串数组
*/
std::vector<std::string> stringSplit(const std::string& str, const char delimiter, bool keepEmpty = false);


}// namespace UTIL