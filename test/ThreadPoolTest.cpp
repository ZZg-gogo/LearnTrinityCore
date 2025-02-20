#include "../Threading/ThreadPool.h"
#include <atomic>
#include <iostream>


class Test
{
public:
    Test()
    {}

    void operator()()
    {
        std::cout << "count_ = "<<++count_ << std::endl;
    }
private:
    static std::atomic_int32_t count_;
};


std::atomic_int32_t Test::count_ = 0;

int main()
{
    THREADING::ThreadPool pool(2);
    
    for (int i = 0; i < 100000; ++i) 
    {
        pool.post(Test());
    }

    pool.join();    //等到所有任务完成之后退出
    return 0;
}

