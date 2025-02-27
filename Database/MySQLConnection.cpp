#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mysql/mysqld_error.h>
#include <mysql/errmsg.h>
#include <mysql/mysql.h>
#include <spdlog/spdlog.h>
#include <thread>

#include "MySQLConnection.h"
#include "../Utilities/Util.h"
#include "MySQLHacks.h"
#include "QueryResult.h"
#include "MySQLPreparedStatement.h"
#include "DatabaseWorker.h"


namespace DATABASE 
{

MysqlConnectionInfo::MysqlConnectionInfo(const std::string& info)
{
    auto result = UTIL::stringSplit(info, ';');
    if (result.size() < 5) 
    {
        spdlog::error("MysqlConnectionInfo info error {}", info);
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


MysqlConnection::MysqlConnection(MysqlConnectionInfo& info) :
    reconnecting_(false),
    prepareError_(false),
    mysqlHandler_(nullptr),
    connectionInfo_(info),
    connectionFlags_(CONNECTION_SYNCH)
{

}


/**
* @brief 构造函数
* @param queue sql任务队列
* @param info 数据库连接信息 异步
*/
MysqlConnection::MysqlConnection(THREADING::ProducerConsumerQueue<SQLOperation*>* queue,
     MysqlConnectionInfo& info) :
    reconnecting_(false),
    prepareError_(false),
    mysqlHandler_(nullptr),
    connectionInfo_(info),
    connectionFlags_(CONNECTION_ASYNC),
    sqlQueue_(queue)
{  
    worker_ = std::make_unique<DatabaseWorker>(this, sqlQueue_);
}


MysqlConnection::~MysqlConnection()
{
    close();
}

/**
* @brief 打开数据库连接
* @return uint32_t 连接状态
*/
uint32_t MysqlConnection::open()
{
    MYSQL * mysqlInit;
    mysqlInit = mysql_init(nullptr);
    if (!mysqlInit)
    {
        spdlog::error("mysql_init open failed {}", connectionInfo_.database);
        return CR_UNKNOWN_ERROR;
    }

    int port;
    char const* unixSocket;
    //在连接之前调用 设置mysql客户端的字符集
    mysql_options(mysqlInit, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    //根据需要选择是 使用TCP/IP协议还是本地套接字协议
    if (connectionInfo_.host == ".") 
    {
        unsigned int opt = MYSQL_PROTOCOL_SOCKET;
        mysql_options(mysqlInit, MYSQL_OPT_PROTOCOL, &opt);
        connectionInfo_.host = "localhost";
        port = 0;
        unixSocket = connectionInfo_.port.c_str();
    }
    else 
    {
        port = std::stoi(connectionInfo_.port);
        unixSocket = nullptr;
    }

    //连接到数据库
    mysqlHandler_ = reinterpret_cast<MySQLHandle*>(mysql_real_connect(mysqlInit, 
        connectionInfo_.host.c_str(), 
        connectionInfo_.user.c_str(),
        connectionInfo_.password.c_str(),
        connectionInfo_.database.c_str(),
        port, unixSocket, 0));

    //连接成功的话
    if (mysqlHandler_) 
    {
        spdlog::info("mysql_real_connect open success {}:{}", connectionInfo_.host, connectionInfo_.database);

        if (!reconnecting_) 
        {
            spdlog::info("open sql.sql MySQL client library: {}", mysql_get_client_info());
            spdlog::info("open sql.sql MySQL server ver: {} ", mysql_get_server_info(mysqlHandler_));
        }

        mysql_autocommit(mysqlHandler_, 1);

        //设置当前连接的默认字符集
        mysql_set_character_set(mysqlHandler_, "utf8mb4");
        return 0;
    }
    else
    {
        spdlog::error("mysql_real_connect open failed {}:{}", connectionInfo_.host, mysql_error(mysqlInit));
        //返回mysql的错误码
        uint32_t errorCode = mysql_errno(mysqlInit);
        mysql_close(mysqlInit);
        return errorCode;
    }

    
}

void MysqlConnection::close()
{
    if (mysqlHandler_) 
    {
        mysql_close(mysqlHandler_);
        mysqlHandler_ = nullptr;
    }
    
}


/**
* @brief sync执行sql语句
* @param sql 待执行的sql语句
* @return bool 执行结果
*/
bool MysqlConnection::Execute(const char * sql)
{
    if (!mysqlHandler_) 
    {
        return false;
    }

    if (mysql_query(mysqlHandler_, sql)) 
    {
        uint32_t errorCode = mysql_errno(mysqlHandler_);
        spdlog::error("Execute sql.sql SQL: {}", sql);
        spdlog::error("Execute sql.sql  [{}] {}", errorCode, mysql_error(mysqlHandler_));

        //根据错误码 处理错误 如果返回结果是true,那么尝试再次执行sql
        if (_HandleMySQLErrno(errorCode)) 
        {
            return Execute(sql);
        }

        return false;
    }
    else
    {
        spdlog::debug("MysqlConnection::Execute succ sql:{}", sql);
    }

    return true;
}


/**
* @brief 查询sql语句
* @param sql 待查询的sql语句
* @return ResultSet * 查询结果
*/
ResultSet * MysqlConnection::Query(const char * sql)
{
    if (!sql) 
    {
        return nullptr;
    }

    MySQLResult * result = nullptr;
    MySQLField * fields = nullptr;
    uint64_t numRows = 0;
    uint32_t numFields = 0;


    if (_Query(sql, &result, &fields, &numRows, &numFields)) 
    {
        return new ResultSet(result, fields, numRows, numFields);
    }

    
    return nullptr;
}


/**
* @brief 查询sql语句
* @param sql 待查询的sql语句
* @param pResult 查询结果
* @param pFields 查询的字段
* @param pRowCount 查询的行数
* @param pFieldCount 查询的字段数
*/
bool MysqlConnection::_Query(char const* sql, MySQLResult** pResult, MySQLField** pFields, uint64_t* pRowCount, uint32_t* pFieldCount)
{
    if (!mysqlHandler_) 
    {
        return false;
    }

    //mysql查询出现问题了
    if (mysql_query(mysqlHandler_, sql)) 
    {
        uint32_t errorCode = mysql_errno(mysqlHandler_);
        spdlog::error("MysqlConnection::_Query sql.sql SQL: {}", sql);
        spdlog::error("MysqlConnection::_Query sql.sql  [{}] {}", errorCode, mysql_error(mysqlHandler_));
        //错误处理后 进行重试
        if (_HandleMySQLErrno(errorCode)) 
        {
            return _Query(sql, pResult, pFields, pRowCount, pFieldCount);
        }

        return false;
    }

    spdlog::debug("MysqlConnection::_Query succ sql:{}", sql);

    //获取查询结果
    *pResult = reinterpret_cast<MySQLResult*>(mysql_store_result(mysqlHandler_));
    //获取查询的行数
    *pRowCount = mysql_affected_rows(mysqlHandler_);
    //获取查询的字段数
    *pFieldCount = mysql_field_count(mysqlHandler_);

    if (!*pResult) 
    {
        return false;
    }

    if (!*pRowCount) 
    {
        mysql_free_result(*pResult);
        return false;
    }

    *pFields = reinterpret_cast<MySQLField*>(mysql_fetch_fields(*pResult));

    return true;
}



/**
* @brief 执行预处理sql语句
* @param base 预处理语句
* @return PreparedResultSet* 查询结果
*/
PreparedResultSet* MysqlConnection::Query(PreparedStatementBase * base)
{


    MySQLPreparedStatement * mysqlStmt = nullptr;
    MySQLResult * result = nullptr;
    uint64_t rowCount = 0;
    uint32_t fieldCount = 0;

    if (!_Query(base,
        &mysqlStmt,
        &result,
        &rowCount,
        &fieldCount)) 
    {
        return nullptr;
    }

    if (mysql_more_results(mysqlHandler_))
    {
        mysql_next_result(mysqlHandler_);
    }

    return new PreparedResultSet (mysqlStmt->getSTMT(), result, rowCount, fieldCount);

}


/**
* @brief 查询预处理sql语句
* @param stmt 预处理语句
* @param mysqlStmt mysql预处理语句
* @param pResult 查询结果
* @param pRowCount 查询的行数
* @param pFieldCount 查询的字段数
*/
bool MysqlConnection::_Query(PreparedStatementBase* base,
    MySQLPreparedStatement** mysqlStmt,
    MySQLResult** pResult,
    uint64_t* pRowCount,
    uint32_t* pFieldCount)
{
    if (!mysqlHandler_) 
    {
        return false;
    }


    //获取预处理语句的索引
    uint32_t stmtIndex =  base->GetIndex();
    //获取预处理语句
    MySQLPreparedStatement* pMysqlStmt = GetPreparedStatement(stmtIndex);
    //绑定参数
    pMysqlStmt->bindPrameters(base);

    *mysqlStmt = pMysqlStmt;
    MYSQL_STMT* msqlSTMT = pMysqlStmt->getSTMT();
    MYSQL_BIND* msqlBIND = pMysqlStmt->getBind();


    if (mysql_stmt_bind_param(msqlSTMT, msqlBIND))
    {
        uint32_t lErrno = mysql_errno(mysqlHandler_);
        spdlog::error("MysqlConnection::_Query mysql_stmt_bind_param   [ERROR]: [{}] {}", lErrno, mysql_stmt_error(msqlSTMT));

        if (_HandleMySQLErrno(lErrno)) 
        {
            return _Query(base, mysqlStmt, pResult, pRowCount, pFieldCount);
        }

        pMysqlStmt->clearParameters();
        return  false;
    }

    if (mysql_stmt_execute(msqlSTMT)) 
    {
        uint32_t lErrno = mysql_errno(mysqlHandler_);
        spdlog::error("MysqlConnection::_Query mysql_stmt_execute   [ERROR]: [{}] {}", lErrno, mysql_stmt_error(msqlSTMT));

        if (_HandleMySQLErrno(lErrno)) 
        {
            return _Query(base, mysqlStmt, pResult, pRowCount, pFieldCount);
        }

        pMysqlStmt->clearParameters();
        return  false;
    }

    spdlog::info("MysqlConnection::_Query SUCC {}", pMysqlStmt->getQueryString());
    pMysqlStmt->clearParameters();
    
    *pResult = reinterpret_cast<MySQLResult*>(mysql_stmt_result_metadata(msqlSTMT));
    *pRowCount = mysql_stmt_num_rows(msqlSTMT);
    *pFieldCount = mysql_stmt_field_count(msqlSTMT);

    return true;
}


/**
* @brief 新增预处理sql语句
* @param index 预处理语句的索引
* @param sql 待处理的sql语句
* @param flags 连接标志
*/
void MysqlConnection::PreparedStatement(uint32_t index, const std::string& sql, ConnectionFlags flags)
{
    if (! (connectionFlags_&flags)) 
    {
        preparedStatements_[index].reset();
        return;
    }

    if (index >= preparedStatements_.size()) 
    {
        preparedStatements_.resize((preparedStatements_.size()+1) * 2);
    }


    MYSQL_STMT* stmt = mysql_stmt_init(mysqlHandler_);

    if (!stmt) 
    {
        spdlog::error("PreparedStatement sql.sql Could not initialize statement {}", sql);
        spdlog::error("sql.sql", "{}", mysql_error(mysqlHandler_));
        prepareError_ = true;
        return;
    }

    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()))
    {
        mysql_stmt_close(stmt);
        prepareError_ = true;
        return;
    }
    

    preparedStatements_[index] = std::make_unique<MySQLPreparedStatement>(reinterpret_cast<MySQLStmt*>(stmt), sql);
}


/**
* @brief sync执行预处理语句
* @param base 预处理语句
* @return bool 执行结果
*/
bool MysqlConnection::Execute(PreparedStatementBase * base)
{

    if (!mysqlHandler_) 
    {
        return false;
    }
    //获取预处理语句的索引
    uint32_t stmtIndex =  base->GetIndex();
    //获取预处理语句
    MySQLPreparedStatement* pMysqlStmt = GetPreparedStatement(stmtIndex);
    //绑定参数
    pMysqlStmt->bindPrameters(base);


    MYSQL_STMT* msqlSTMT = pMysqlStmt->getSTMT();
    MYSQL_BIND* msqlBIND = pMysqlStmt->getBind();


    if (mysql_stmt_bind_param(msqlSTMT, msqlBIND))
    {
        uint32_t lErrno = mysql_errno(mysqlHandler_);
        spdlog::error("MysqlConnection::Execute mysql_stmt_bind_param   [ERROR]: [{}] {}", lErrno, mysql_stmt_error(msqlSTMT));

        if (_HandleMySQLErrno(lErrno)) 
        {
            return Execute(base);
        }

        pMysqlStmt->clearParameters();
        return  false;
    }

    if (mysql_stmt_execute(msqlSTMT)) 
    {
        uint32_t lErrno = mysql_errno(mysqlHandler_);
        spdlog::error("MysqlConnection::Execute mysql_stmt_execute   [ERROR]: [{}] {}", lErrno, mysql_stmt_error(msqlSTMT));

        if (_HandleMySQLErrno(lErrno)) 
        {
            return Execute(base);
        }

        pMysqlStmt->clearParameters();
        return  false;
    }

    spdlog::info("MysqlConnection::Execute SUCC {}", pMysqlStmt->getQueryString());
    pMysqlStmt->clearParameters();
    return true;
}




void MysqlConnection::ping()
{
    mysql_ping(mysqlHandler_);
}

uint32_t MysqlConnection::getLastError() const
{
    return mysql_errno(mysqlHandler_);
}

/**
* @brief 获取预处理语句
* @param index 预处理语句的索引
* @return MySQLPreparedStatement* 预处理语句
*/
MySQLPreparedStatement* MysqlConnection::GetPreparedStatement(uint32_t index)
{
    return preparedStatements_[index].get();
}


/**
* @brief 预处理sql语句
*/
bool MysqlConnection::PrepareStatements()
{
    doPrepareStatements();
    return !prepareError_;
}

void MysqlConnection::doPrepareStatements()
{
    
}


/**
* @brief 处理mysql错误
* @param errNo 错误码
* @param attempts 尝试次数
* @return bool 错误处理结果
*/
bool MysqlConnection::_HandleMySQLErrno(uint32_t errNo, uint8_t attempts)
{

    switch (errNo) 
    {
        case CR_SERVER_GONE_ERROR:  
        //表示与 MySQL 服务器的连接已丢失
        case CR_SERVER_LOST:        
        //表示在查询过程中与 MySQL 服务器的连接丢失
        case CR_SERVER_LOST_EXTENDED:
        /*表示在查询过程中与 MySQL 服务器的连接丢失，并提供了更多的错误信息*/
        {
            if (mysqlHandler_) 
            {
                spdlog::error("_HandleMySQLErrno sql.sql Lost connection to MySQL server");
                mysql_close(mysqlHandler_);
                mysqlHandler_ = nullptr;
            }

            [[fallthrough]];//执行下面一个case
        }
        case CR_CONN_HOST_ERROR:
        //表示无法连接到 MySQL 服务器主机
        {
            spdlog::error("_HandleMySQLErrno sql.sql Attempting to connect to MySQL server");

            reconnecting_ = true;
            const uint32_t hasError = open();
            //重连成功的话
            if (!hasError) 
            {   
                //进行预处理失败的话
                if (!this->PrepareStatements()) 
                {
                    spdlog::error("_HandleMySQLErrno sql.sql Could not re-prepare statements!");
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    abort();
                }

                
                spdlog::info("_HandleMySQLErrno sql.sql Successfully reconnected to {} @{}:{} ({}).",
                    connectionInfo_.database, connectionInfo_.host, connectionInfo_.port,
                        (connectionFlags_ & CONNECTION_ASYNC) ? "asynchronous" : "synchronous");
                

                reconnecting_ = false;
                return true;
            }

            //尝试之后发现已经连接不上mysql了
            if ((--attempts) == 0) 
            {
                spdlog::error("_HandleMySQLErrno sql.sql Failed to reconnect to the MySQL server, "
                    "terminating the server to prevent data corruption!");
                
                std::this_thread::sleep_for(std::chrono::seconds(10));
                abort();
            }
            else //再次进行重连尝试
            {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                return _HandleMySQLErrno(errNo, attempts);
            }

            break;
        }
        
        case ER_LOCK_DEADLOCK:
        //表示发生了死锁
        {
            return false;
        }
        
        case ER_WRONG_VALUE_COUNT:
        //表示查询中的值数量不正确
        case ER_DUP_ENTRY:
        //表示插入的数据违反了唯一约束
        {
            return false;
        }
            
        case ER_BAD_FIELD_ERROR:
        //字段错误
        case ER_NO_SUCH_TABLE:
        //查询了不存在的表
        {
            spdlog::error("_HandleMySQLErrno Your database structure is not up to date. Please make sure you've executed all queries in the sql/updates folders.");
            std::this_thread::sleep_for(std::chrono::seconds(10));
            abort();
            return false;
        }

        case ER_PARSE_ERROR:
        //sql解析错误
        {
            spdlog::error("_HandleMySQLErrno sql.sql Error while parsing SQL. Core fix required.");
            std::this_thread::sleep_for(std::chrono::seconds(10));
            abort();
            return false;
        }

        default:
        {
            spdlog::error("_HandleMySQLErrno sql.sql Unhandled MySQL errno {}. Unexpected behaviour possible.", errNo);
            return false;
        }

    }

    return true;
}


}// namespace DATABASE 