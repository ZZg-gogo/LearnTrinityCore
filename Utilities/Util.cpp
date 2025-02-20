#include "Util.h"

namespace UTIL
{

/**
* @brief 字符串分割
* @param str 待分割字符串
* @param delimiter 分隔符
* @param keepEmpty 是否保留空字符串
* @return 分割后的字符串数组
*/
std::vector<std::string> stringSplit(const std::string& str, const char delimiter, bool keepEmpty)
{
    std::vector<std::string> result;

    std::string::size_type start = 0;

    for (std::string::size_type end = str.find((delimiter)); 
        end != std::string::npos;
        end = str.find((delimiter))) 
    {
        if (keepEmpty || start < end) 
        {
            result.push_back(str.substr(start, end-start));
        }

        ++start;
    }

    if (keepEmpty || start < str.length()) 
    {
        result.push_back(str.substr(start));
    }


    return std::move(result);
}


}// namespace UTIL