#include <mutex>
#include <thread>
#include <queue>
#include <iostream>
#include <future>
#include <string>
#include <functional>
#include "threadsafe_queue.h"

bool flag = false;
std::mutex m;
void wait_for_flag()
{
    std::unique_lock<std::mutex> lk(m, std::defer_lock);
    while(!flag)
    {
        lk.lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "wait!!" << std::endl;
        lk.unlock();
    }
}

struct data_chunk { int num = 1994; };

std::mutex mut;
std::queue<data_chunk> data_queue;
std::condition_variable data_cond;

void data_preparation_thread()
{
    data_chunk data; // prepare_data

    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(data);
    }

    std::cout << "[data_preparation_thread] notify" << std::endl;
    data_cond.notify_one();
}

void data_processing_thread()
{
    // condition_variable을 사용해서, promise/future처럼 생산자/소비자 패턴을 구현할 수 있음.
    std::unique_lock<std::mutex> lk(mut);
    std::cout << "[data_processing_thread] wait" << std::endl;
    data_cond.wait(lk, []()
    { 
        std::cout << "data_cond.wait : " << data_queue.empty() << std::endl;
        return !data_queue.empty(); 
    });

    data_chunk data = data_queue.front();
    data_queue.pop();
    lk.unlock();
    std::cout << "data num : " << data.num << std::endl;
}

struct X
{
    int foo(int num, std::string const& str)
    {
        std::cout << "num : " << num << ", str : " << str << std::endl;
        return num;
    }

    std::string bar(std::string const& str)
    {
        std::cout << "str : " << str << std::endl;
        return str;
    }
};

struct Y
{
    double operator()(double num)
    {
        std::cout << num << std::endl;
        return num;
    }
};

X baz(X& x)
{
    std::cout << "baz" << std::endl;
}

void worker(std::promise<std::string>* p) 
{
    // 해당 결과는 future 에 들어간다.
    p->set_value("some data");
}

class move_only
{
public:
    move_only();
    move_only(move_only&&);
    move_only(move_only const&) = delete;
    move_only& operator=(move_only&&);
    move_only& operator=(move_only const&) = delete;
    void operator()();
};

int main()
{
    // 다른 상태 혹은 event 대기 - 1
    std::thread waitForFlag(wait_for_flag);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    flag = true;
    waitForFlag.join();
    std::cout << "wait done" << std::endl;

    // 다른 상태 혹은 event 대기 - 2
    std::thread proccessThread(data_processing_thread);
    std::thread prepareThread(data_preparation_thread);

    proccessThread.join();
    prepareThread.join();

    // Thread Safe Queue
    threadsafe_queue<int> safeQ;
    
    std::thread safeQPushThread([&safeQ]()
    { 
        for(int i = 0; i < 10; i++)
        {
            safeQ.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread safeQWaitPopThread([&safeQ]()
    { 
        for(int i = 0; i < 10; i++)
        {
            std::cout << *safeQ.wait_and_pop() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    safeQPushThread.join();
    safeQWaitPopThread.join();

    // std::promise, std::future
    std::promise<std::string> p;

    // 미래에 string 데이터를 돌려 주겠다는 약속.
    std::future<std::string> data = p.get_future();
    std::thread promiseThread(worker, &p);
    
    // 미래에 약속된 데이터를 받을 때 까지 기다린다.
    data.wait();
    // wait 이 리턴했다는 뜻이 future 에 데이터가 준비되었다는 의미.
    // 참고로 wait 없이 그냥 get 해도 wait 한 것과 같다.
    std::cout << "받은 데이터 : " << data.get() << std::endl;
    promiseThread.join();

    // 정리해보면, promise 는 생산자-소비자 패턴에서 마치 생산자 (producer) 의 역할을 수행하고, 
    // future 는 소비자 (consumer) 의 역할을 수행함.

    // std::async 사용 시, std::future로 값을 받을 때까지 wait로 대기 가능하다
    X x;

    std::future<int> f1 = std::async(std::launch::deferred, &X::foo, &x, 1, "hello1");
    std::future<int> f2 = std::async(std::launch::async, &X::foo, &x, 2, "hello2");
    std::future<int> f3 = std::async(&X::foo, &x, 3, "hello3");

    std::cout << f1.get() << std::endl;

    // std::async 인자 전달
    {
        X x;
        auto f1 = std::async(&X::foo, &x, 1, "hello"); // &x foo 호출. (x의 foo 호출)
        auto f2 = std::async(&X::bar, x, "goodbye"); // x의 복사본에 있는 bar 호출
    }

    {
        Y y;
        auto f3 = std::async(Y(), 3.141); // Y()로부터 이동 생성되어진 object의 operator() 호출
        auto f4 = std::async(std::ref(y), 2.718); // 164 line의 y의 operator() 호출
    }

    {
         std::async(baz, std::ref(x)); // call baz(x)
    }

    return 0;
}