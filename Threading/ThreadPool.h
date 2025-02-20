#pragma once


#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <thread>
#include <utility>

namespace THREADING 
{


/**
* @class ThreadPool
* @brief boost::asio::thread_pool的简单封装
*/

class ThreadPool
{
public:
    using ptr = std::shared_ptr<ThreadPool>;

public:
    /**
    * @brief 构造函数
    * @param numThreads 开启多少个线程
    */

    explicit ThreadPool(std::size_t numThreads) : pool_(numThreads)
    {}


    /**
    * @brief 向线程池提交一个任务
    * @param task 任务
    */
    template<typename T>
    decltype(auto) post(T&& task)
    {
        return boost::asio::post(pool_, std::forward<T>(task));
    }

    /**
    * @brief 阻塞调用线程 等待所有任务完成之后退出
    */
    void join()
    {
        pool_.join();
    }

    /**
    * @brief 终止任务运行
    */
    void stop()
    {
        pool_.stop();
    }

private:
    boost::asio::thread_pool pool_;
};

}// end  namespace Threading