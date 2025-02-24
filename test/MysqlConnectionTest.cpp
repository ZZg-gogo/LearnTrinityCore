#include "../Database/MySQLConnection.h"
#include <iostream>
#include <mysql/mysql.h>
#include <spdlog/spdlog.h>
#include "../Database/QueryResult.h"

#define MYSQL_INFO "127.0.0.1;3306;zzh;123456;test;enable"

int main()
{
    DATABASE::MysqlConnectionInfo info(MYSQL_INFO);

    std::cout<<"host:"<<info.host<<"port:"<<info.port<<"user:"
        <<info.user<<"password:"<<info.password<<"database:"<<info.database<<"ssl:"<<info.ssl<<std::endl;


    DATABASE::MysqlConnection conn(info);
    conn.open();

    
    
    //conn.Execute("insert into stu values(null, 'th', 20)");


    DATABASE::ResultSet * pResultSet =  conn.Query("select * from stu");

    spdlog::info("row count:{}", pResultSet->GetRowCount());

    while (pResultSet->nextRow()) 
    {
        DATABASE::Field* pField =  pResultSet->fetch();

        spdlog::info("id:{}, name:{}, age:{}", pField[0].getInt32(), pField[1].getString(), pField[2].getInt32());
    }

    return 0;
}