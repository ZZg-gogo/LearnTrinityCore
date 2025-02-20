#include "MySQLConnection.h"
#include "../Utilities/Util.h"

namespace DATABASE 
{

MysqlConnectionInfo::MysqlConnectionInfo(const std::string& info)
{
    auto result = UTIL::stringSplit(info, ';');
    if (result.size() < 5) 
    {
        return;
    }

    host = result[0];
    port = result[1];
    user = result[2];
    password = result[3];
    database = result[4];

    if (result.size() == 6) 
    {
        ssl = result[5];
    }
    
}


}// namespace DATABASE 