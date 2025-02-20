#pragma once


#include <algorithm>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <type_traits>

namespace THREADING 
{


/**
* @class ProducerConsumerQueue
* @brief 生产者消费者队列
*/
template <typename T>
class ProducerConsumerQueue
{
public:
    using ptr = std::shared_ptr<ProducerConsumerQueue>;
public:
    /**
    * @brief 构造函数
    */
    ProducerConsumerQueue() : stop_(false)
    {}

    /**
    * @brief 放入一个元素
    * @param v 元素
    */
    void push(T&& v)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!stop_) 
        {
            queue_.push(std::move(v));
            cond_.notify_one();
        }
    }

    /**
    * @brief 放入一个元素
    * @param v 元素
    */
    void push(const T& v)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!stop_) 
        {
            queue_.push(v);
            cond_.notify_one();
        }
    }

    /**
    * @brief 获取一个元素
    * @param v 元素
    * @return 是否成功获取
    */

    bool get(T& v)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        while (queue_.empty() && !stop_) 
        {
            cond_.wait(lock);
        }

        if (queue_.empty() || stop_) 
        {
            return false;
        }

        v = std::move(queue_.front());
        queue_.pop();

        return true;
    }

    /**
    * @brief 获取队列大小
    * @return 队列大小
    */
    std::size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
    * @brief 停止队列 并清空任务
    */
    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;

            while (!queue_.empty()) 
            {
                T v = queue_.front();
                
                if constexpr (std::is_pointer_v<T>) 
                {
                    delete v;
                }
                queue_.pop();
            }
        }

        cond_.notify_all();
    }

private:

private:
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<T> queue_;
    bool stop_;
}; 


}