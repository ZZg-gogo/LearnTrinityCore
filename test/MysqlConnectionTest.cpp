#include "../Database/MySQLConnection.h"
#include <iostream>


#define MYSQL_INFO "127.0.0.1;3306;zzh;123456;test;enable"

int main()
{
    DATABASE::MysqlConnectionInfo info(MYSQL_INFO);

    std::cout<<"host:"<<info.host<<"port:"<<info.port<<"user:"
        <<info.user<<"password:"<<info.password<<"database:"<<info.database<<"ssl:"<<info.ssl<<std::endl;


    DATABASE::MysqlConnection conn(info);
    conn.open();


    return 0;
}