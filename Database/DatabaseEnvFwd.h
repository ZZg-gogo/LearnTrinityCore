#pragma once

#include <future>
#include <memory>


namespace DATABASE 
{

class ResultSet;
using QueryResult = std::shared_ptr<ResultSet>;
using QueryResultFuture = std::future<QueryResult>;
using QueryResultPromise = std::promise<QueryResult>;

class PreparedResultSet;
using PreparedQueryResult = std::shared_ptr<PreparedResultSet>;
using PreparedQueryResultFuture = std::future<PreparedQueryResult>;
using PreparedQueryResultPromise = std::promise<PreparedQueryResult>;


}