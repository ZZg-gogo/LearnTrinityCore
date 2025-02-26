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

    
    DATABASE::PreparedStatementBase pBase{0, 2};
    conn.PreparedStatement(0, "insert into stu values(null, ?, ?)", DATABASE::CONNECTION_SYNCH);
    pBase.setString(0, "zzh");
    pBase.setInt32(1, 25);
    conn.Execute(&pBase);

    
    DATABASE::PreparedStatementBase pBase1{1, 1};
    conn.PreparedStatement(1, "select * from stu where name = ?", DATABASE::CONNECTION_SYNCH);
    pBase1.setString(0, "zzh");
    DATABASE::PreparedResultSet* pPrepraredres =  conn.Query(&pBase1);
    
    
    do {
        DATABASE::Field* pField =  pPrepraredres->Fetch();

        spdlog::info("id:{}, name:{}, age:{}", pField[0].getInt32(), pField[1].getString(), pField[2].getInt32());
    }while (pPrepraredres->nextRow());
    return 0;
}