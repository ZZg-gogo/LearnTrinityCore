#pragma once


#include <string>



/**
* @file MySQLConnection.h
* @brief Myssql数据库连接
*/



namespace  DATABASE
{

struct MysqlConnectionInfo
{
    explicit MysqlConnectionInfo(const std::string& info);

    std::string host;
    std::string port;
    std::string user;
    std::string password;
    std::string database;
    std::string ssl;
};



}// namespace  DATABASE