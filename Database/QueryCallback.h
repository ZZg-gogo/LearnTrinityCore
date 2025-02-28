#pragma once


#include "DatabaseEnvFwd.h"
#include <memory>
#include <queue>
#include <list>

namespace DATABASE 
{

/**
* @class QueryCallback
* @brief 对异步请求的返回结果进行处理
*/
class QueryCallback
{
public:
    using ptr = std::shared_ptr<QueryCallback>;
public:
    explicit QueryCallback(QueryResultFuture&& result);
    explicit QueryCallback(PreparedQueryResultFuture&& result);

    QueryCallback(QueryCallback&& right);
    QueryCallback& operator=(QueryCallback&& right);
    ~QueryCallback();

    //添加回调函数
    QueryCallback&& WithCallback(std::function<void(QueryResult)>&& callback);
    QueryCallback&& WithPreparedCallback(std::function<void(PreparedQueryResult)>&& callback);
    QueryCallback&& WithChainingCallback(std::function<void(QueryCallback&, QueryResult)>&& callback);
    QueryCallback&& WithChainingPreparedCallback(std::function<void(QueryCallback&, PreparedQueryResult)>&& callback);

    void SetNextQuery(QueryCallback&& next);

    /**
    * @brief 如果查询结果已经准备好，则调用回调函数
    * @return 如果所有的回调函数已经被执行完毕 那么返回true 否则返回false
    */
    bool InvokeIfReady();
    
private:
    QueryCallback(QueryCallback const& right) = delete;
    QueryCallback& operator=(QueryCallback const& right) = delete;

    template<typename T> friend void ConstructActiveMember(T* obj);
    template<typename T> friend void DestroyActiveMember(T* obj);
    template<typename T> friend void MoveFrom(T* to, T&& from);

private:
    union 
    {
        QueryResultFuture string_;  //查询结果的future对象
        PreparedQueryResultFuture prepared_;    //预处理结果的future对象
    };

    bool isPrepared_;   //是否是预处理语句

    struct QueryCallbackData;

    std::queue<QueryCallbackData, std::list<QueryCallbackData>> callbacks_;    //回调函数队列
};    


}