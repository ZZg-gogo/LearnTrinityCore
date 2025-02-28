#include "QueryCallback.h"
#include "DatabaseEnvFwd.h"
#include <algorithm>
#include <chrono>


namespace DATABASE 
{
template<typename T, typename... Args>
inline void Construct(T& t, Args&&... args)
{
    new (&t) T(std::forward<Args>(args)...);
}

template<typename T>
inline void Destroy(T& t)
{
    t.~T();
}

template<typename T>
inline void ConstructActiveMember(T* obj)
{
    if (!obj->isPrepared_)
        Construct(obj->string_);
    else
        Construct(obj->prepared_);
}

template<typename T>
inline void DestroyActiveMember(T* obj)
{
    if (!obj->isPrepared_)
        Destroy(obj->string_);
    else
        Destroy(obj->prepared_);
}

template<typename T>
inline void MoveFrom(T* to, T&& from)
{
    if (!to->isPrepared_)
        to->string_ = std::move(from.string_);
    else
        to->prepared_ = std::move(from.prepared_);
}



struct QueryCallback::QueryCallbackData
{
public:
    friend class QueryCallback;

    QueryCallbackData(std::function<void(QueryCallback&, QueryResult)>&& callback) : 
    string_(std::move(callback)), isPrepared_(false) 
    { }
    QueryCallbackData(std::function<void(QueryCallback&, PreparedQueryResult)>&& callback) :
    prepared_(std::move(callback)), isPrepared_(true) { }

     
    QueryCallbackData(QueryCallbackData&& right)
    {
        isPrepared_ = right.isPrepared_;
        ConstructActiveMember(this);
        MoveFrom(this, std::move(right));
    }
    QueryCallbackData& operator=(QueryCallbackData&& right)
    {
        if (this != &right)
        {
            if (isPrepared_ != right.isPrepared_)
            {
                DestroyActiveMember(this);
                isPrepared_ = right.isPrepared_;
                ConstructActiveMember(this);
            }
            MoveFrom(this, std::move(right));
        }
        return *this;
    }
    ~QueryCallbackData() { DestroyActiveMember(this); }

private:
    QueryCallbackData(QueryCallbackData const&) = delete;
    QueryCallbackData& operator=(QueryCallbackData const&) = delete;

    template<typename T> friend void ConstructActiveMember(T* obj);
    template<typename T> friend void DestroyActiveMember(T* obj);
    template<typename T> friend void MoveFrom(T* to, T&& from);

    union
    {
        std::function<void(QueryCallback&, QueryResult)> string_;
        std::function<void(QueryCallback&, PreparedQueryResult)> prepared_;
    };
    bool isPrepared_;
};


QueryCallback::QueryCallback(QueryResultFuture&& result)
{
    isPrepared_ = false;
    Construct(string_, std::move(result));
}
QueryCallback::QueryCallback(PreparedQueryResultFuture&& result)
{
    isPrepared_ = true;
    Construct(prepared_, std::move(result));
}

QueryCallback::QueryCallback(QueryCallback&& right)
{
    isPrepared_ = right.isPrepared_;
    ConstructActiveMember(this);
    MoveFrom(this, std::move(right));
    callbacks_ = std::move(right.callbacks_);
}


QueryCallback& QueryCallback::operator=(QueryCallback&& right)
{
    if (this != &right)
    {
        if (isPrepared_ != right.isPrepared_)
        {
            DestroyActiveMember(this);
            isPrepared_ = right.isPrepared_;
            ConstructActiveMember(this);
        }
        MoveFrom(this, std::move(right));
        callbacks_ = std::move(right.callbacks_);
    }
    return *this;
}

QueryCallback::~QueryCallback()
{
    DestroyActiveMember(this);
}

QueryCallback&& QueryCallback::WithCallback(std::function<void(QueryResult)>&& callback)
{
    return WithChainingCallback([callback](QueryCallback& /*this*/, QueryResult result) { callback(std::move(result)); });
}

QueryCallback&& QueryCallback::WithPreparedCallback(std::function<void(PreparedQueryResult)>&& callback)
{
    return WithChainingPreparedCallback([callback](QueryCallback& /*this*/, PreparedQueryResult result) { callback(std::move(result)); });
}

QueryCallback&& QueryCallback::WithChainingCallback(std::function<void(QueryCallback&, QueryResult)>&& callback)
{
    callbacks_.emplace(std::move(callback));
    return std::move(*this);
}

QueryCallback&& QueryCallback::WithChainingPreparedCallback(std::function<void(QueryCallback&, PreparedQueryResult)>&& callback)
{
    callbacks_.emplace(std::move(callback));
    return std::move(*this);
}

void QueryCallback::SetNextQuery(QueryCallback&& next)
{
    MoveFrom(this, std::move(next));
}

/**
* @brief 如果查询结果已经准备好，则调用回调函数
* @return 如果所有的回调函数已经被执行完毕 那么返回true 否则返回false
*/
bool QueryCallback::InvokeIfReady()
{
    QueryCallbackData &callback = callbacks_.front();

    auto checkStateAndReturnCompletion = [this](){
        callbacks_.pop();

        bool hasNext = !isPrepared_ ? string_.valid() : prepared_.valid();

        if (callbacks_.empty()) 
        {
            return true;
        }

        if (!hasNext)
        {
            return true;
        }

        return false;
    };


    if (!isPrepared_ )
    {
        if (string_.valid() && string_.wait_for(std::chrono::seconds(0))   == std::future_status::ready) 
        {
            QueryResultFuture f(std::move(string_));
            std::function<void(QueryCallback&, QueryResult)> cb(std::move(callback.string_));
            cb(*this, f.get());
            return checkStateAndReturnCompletion();
        }
    }
    else
    {
        if (prepared_.valid() && prepared_.wait_for(std::chrono::seconds(0))   == std::future_status::ready) 
        {
            PreparedQueryResultFuture f(std::move(prepared_));
            std::function<void(QueryCallback&, PreparedQueryResult)> cb(std::move(callback.prepared_));
            cb(*this, f.get());
            return checkStateAndReturnCompletion();
        }
    }

    return false;
}

}