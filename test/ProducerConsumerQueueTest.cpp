#include "../Threading/ProducerConsumerQueue.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

class Test
{
public:
    Test()
    {}

    void operator()()
    {
        std::cout <<"xxxcount_ = "<<++count_ << std::endl;
    }
private:
    static std::atomic_int32_t count_;
};


std::atomic_int32_t Test::count_ = 0;

int main()
{
    THREADING::ProducerConsumerQueue<Test>::ptr queue =
        std::make_shared<THREADING::ProducerConsumerQueue<Test>>();

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++) 
    {
        threads.push_back(std::thread([queue](){
            while (true) 
            {
                Test v;

                if (!queue->get(v)) {
                    break;
                }
    
                v();
            }
        }));
    }

    for (int i = 0; i < 100000; ++i) {
        queue->push(Test());
    }

    queue->stop();
    for (auto& i : threads) {
        i.join();
    }

    return 0;
}




