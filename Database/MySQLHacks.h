#pragma  once

#include <mysql/mysql.h>
#include <type_traits>


namespace DATABASE
{

struct MySQLHandle : MYSQL { };
struct MySQLResult : MYSQL_RES { };
struct MySQLField : MYSQL_FIELD { };
struct MySQLBind : MYSQL_BIND { };
struct MySQLStmt : MYSQL_STMT { };


using MySQLBool = std::remove_pointer_t<decltype(std::declval<MYSQL_BIND>().is_null)>;
}

