#include "MySQLThreading.h"

#include <mysql/mysql.h>

namespace DATABASE
{

void MysqlLibraryInit()
{
    mysql_library_init(0, NULL, NULL);
}
void MysqlLibrary_End()
{
    mysql_library_end();
}
unsigned int MysqlGetLibraryVersion()
{
    return MYSQL_VERSION_ID;
}


}// namespace DATABASE