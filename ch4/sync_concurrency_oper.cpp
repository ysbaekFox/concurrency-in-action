#include <mutex>
#include <thread>
#include <queue>
#include <iostream>
#include <future>
#include <string>
#include <functional>
#include <deque>
#include <utility>
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
    move_only() = default;
    move_only(move_only&&) = default;
    move_only(move_only const&) = delete;
    move_only& operator=(move_only&&) = default;
    move_only& operator=(move_only const&) = delete;
    void operator()() { std::cout << "move_only::operator()" << std::endl; }
};

bool g_isShutdownReceived = false;
std::deque<std::packaged_task<void()>> tasks;
bool gui_shutdown_message_received()
{
    return g_isShutdownReceived;
}

void get_and_process_gui_message()
{
    std::cout << "get_and_process_gui_message" << std::endl;
}

void gui_thread()
{
    while(!gui_shutdown_message_received())
    {
        get_and_process_gui_message();
        std::packaged_task<void()> task;
        {
            std::lock_guard<std::mutex> lk(m);
            if(tasks.empty())
                continue;
            task = std::move(tasks.front());
            tasks.pop_back();
        }
        task();

        std::this_thread::sleep_for(std::chrono::microseconds(300));
    }
}

template<typename Func>
std::future<void> post_task_for_gui_thread(Func f)
{
    std::packaged_task<void()> task(f);
    std::future<void> res = task.get_future();
    std::lock_guard<std::mutex> lk(m);
    tasks.push_back(std::move(task));
    return res;
}

void dummyFunc()
{
    std::cout << "dummy Func" << std::endl;
}

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
         auto f5 = std::async(move_only());
    }

    {
        auto f6 = std::async(std::launch::async, Y(), 1.2);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        auto f7 = std::async(std::launch::deferred, baz, std::ref(x));

        // 아마도 any 사용 시, 컴파일러가 더 나은 것을 알아서 선택하게 하는 것 같음.
        // std::launch::deferred | std::launch::async 혹은 std::launch::any
        // 그리고 any는 default와 같다는 것으로 보임. 즉, 아래 f8과 f9는 같다.
        auto f8 = std::async(std::launch::deferred | std::launch::async, baz, std::ref(x));
        auto f9 = std::async(baz, std::ref(x));
        f7.wait();
    }

    // gui_thread에서는 Func을 처리하는 Thread이고,
    // post_task_for_gui_thread는 packaged_task list에 task를 추가하고
    // gui_thread에서 task를 처리하고 나면 반환 받은 future로 get 호출해서 값을 받음.
    std::thread gui_bg_thread(gui_thread);  
    std::future<void> res1 = post_task_for_gui_thread(dummyFunc);
    res1.get();
    std::future<void> res2 = post_task_for_gui_thread(dummyFunc);
    res2.get();
    std::future<void> res3 = post_task_for_gui_thread(dummyFunc);
    res3.get();
    std::future<void> res4 = post_task_for_gui_thread(dummyFunc);
    res4.get();
    g_isShutdownReceived = true;
    gui_bg_thread.join();

    return 0;
}