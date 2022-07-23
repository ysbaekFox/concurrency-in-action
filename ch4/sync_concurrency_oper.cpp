#include <thread>
#include <queue>
#include <iostream>
#include <future>
#include <string>
#include <functional>
#include <deque>
#include <set>
#include <math.h>
#include <utility>
#include <time.h>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

#include "threadsafe_queue.h"
#include "quick_sort.h"

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

class Main
{
public:
    void test()
    {
        std::cout << "test" << std::endl;
    }
};

bool done = false;
std::mutex waitMutex;
std::condition_variable cv;
bool wait_loop()
{
    using namespace std::chrono;
    auto const timeout = steady_clock::now() + milliseconds(500);
    std::unique_lock<std::mutex> lk(m);
    while(!done)
    {
        if(cv.wait_until(lk, timeout) == std::cv_status::timeout)
        {
            std::cout << "timeout!!!" << std::endl;
            done = true;
            break;
        }
    }
    return done;
}

bool cvWaitReady = false;
std::condition_variable cvWaitTest;
bool conditionWaitFor(std::chrono::system_clock::duration d)
{
    std::unique_lock<std::mutex> lk(m);
    cvWaitTest.wait_for(lk, d, []()
    { 
        std::cout << "cvWaitReady : " << cvWaitReady << std::endl;
        return cvWaitReady; 
    });
}

bool conditionWaitUntil(std::chrono::system_clock::time_point t)
{
    std::unique_lock<std::mutex> lk(m);
    cvWaitTest.wait_until(lk, t, []()
    { 
        std::cout << "cvWaitReady : " << cvWaitReady << std::endl;
        return cvWaitReady; 
    });
}

template<typename F, typename A>
std::future<typename std::result_of<F(A&&)>::type> spawn_task(F&& f, A&& a)
{
    // std::packaged_task안에는 함수 시그니처가 들어감.
    // ex) std::packaged_task<int(int, int)> task(add);

    // parameter type의 universal type
    typedef typename std::result_of<F(A&&)>::type result_type;
    std::packaged_task<result_type(A&&)> task(std::move(f));

    // task에서 future object 받아서 return하고
    // task와 parameter value를 move 사용하여 threading.
    std::future<result_type> res(task.get_future());
    std::thread t(std::move(task), std::move(a));
    t.detach();

    return res;
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

    // 많은 네트워크 연결들을 handle하는 것을 필요로 할 때,
    // 각각의 연결을 분리된 스레드에서 처리하곤 한다.
    // 왜냐하면 이것이 더 쉽게 생각하고 더 프로그램을 쉽게 만들기 때문이다.
    // 이것은 적은 수의 연결에서 잘 동작한다. (당연히 적은 수의 스레드를 쓰겠지)
    // 불행하게도, connection의 수가 증가하면, 알맞지 않게 된다. (become less suitable)
    // - 많은 OS resource 소비와 잦은 conteext switching 유발
    // std::future와 std::promise 사용할 수 있다,
    // Listing 4.10 예제..... (근데 잘모르겠다...)

    // saving an exception for the std::future
    auto square_root = [](double x)
    {
        if(x < 0)
        {
            throw std::out_of_range("x < 0");
        }
        return sqrt(x);
    };

    auto calculate_value = [&square_root]()->auto{ return square_root(-1); };

    //double y = square_root(-1); // throw를 던집니다.
    // 그렇다면, 비동기로 실행하면 어떨까요?
    std::future<double> asyncSqureRoot = std::async(square_root, -1);
    asyncSqureRoot.wait(); // wait는 ㄱㅊ음.
    //asyncSqureRoot.get(); // throw 됨 (exception이 future에 저장됨, 그래서 get 호출하면 thorw 발생함.)

    // 또한, std::promise에서도 똑같이 동작함.
    std::promise<double> some_promise;
    auto resultFuture = some_promise.get_future();
    try
    {
        some_promise.set_value(calculate_value());
    }
    catch(...)
    {
#if 0
        some_promise.set_exception(std::current_exception());
#else
        // 요렇게 쓰면 좋다는데... 사실 잘 모르겠음.
        // 코드가 더 깔끔하고, 컴파일러에게 최적화의 기회를 제공한다고 함....
        some_promise.set_exception(std::make_exception_ptr(std::logic_error("x is negative")));
#endif
    }

    try 
    {
        resultFuture.get();
    }
    catch(const std::exception& e) 
    {
        std::cout << "Exception from the thread: " << e.what() << '\n';
    }

    // std::future는 단일 스레드에서만 사용해야 함.
    // 다중 스레드에서는 std::shared_future를 사용해야함.

    // waiting from multi threads
    // std::future의 member function은 not synchronized 함.
    // 다수의 스레드에서 std::future에 추가적인 syncronization 없이 접근하면,
    // data race와 undefined Error를 마주할 것 임.
    // 이것은 std::future가 비동기 결과의 unique ownership으로 설계되었기 때문.
    // 만약 get()을 통해 다중 스레드에서 동시 접근하게 되면 pointless 접근을 하게 될 것임.
    // 왜냐하면 첫번째 get() 호출을 하고 나면, value는 move 되어버리기 때문.

    // std::future는 only moveable
    // std::shared_future는 copyable

    std::promise<int> sp;
    std::future<int> f(sp.get_future()); // get_future는 std::future rvalue를 반환한다!!.
    std::cout << "f valid : " << f.valid() << std::endl;
    std::shared_future<int> sf(std::move(f));
    std::cout << "f valid : " << f.valid() << std::endl;
    std::cout << "f valid : " << f.valid() << std::endl;

    std::cout << "sf valid : " << sf.valid() << std::endl;
    std::shared_future<int> sf2 = sf;
    std::cout << "sf2(copy of sf) valid : " << sf2.valid() << std::endl;

    // std::future는 std::shared_future로 변경할 수 있는 기능을 지원함.
    {
        std::promise<std::string> p;
        std::future<std::string> f = p.get_future();

        // std::promise<std::map<SomeIndex, SomeDataType, SomeComparator, SomeAllocator>::iterator> p;
        // auto sf = p.get_future().shared();

        auto shared_f = f.share();
        std::cout << "shared_f (std::shared_future from f.shared()) valid : " << shared_f.valid() << std::endl;

        std::shared_future<std::string> sf2 = shared_f;
        std::cout << "sf2 ( copy from shared_f ) valid : " << sf2.valid() << std::endl;
    }

    // waiting with time limit
    // 특정 케이스에서 우리는 원하는 만큼만 wait하고 싶을 수 있다. (future의 wait, get 등)
    // std::condition_variable은 wait_for, wait_until을 지원한다.

    // clocks
    // - clock의 시간은 현재이다.
    // - clock value는 clock으로부터 획득한 시간을 나타내는데 사용되는 type
    // - clock은 tick period이다.
    // - clock ticks은 균등한 비율애 대한 것일 수도 아닐 수도 있다. 그러므로 steady를 고려해야한다.

    // clock class는 some_clock::now()를 지원합니다.
    {
        // 참고 : std::chrono::duration<long long, std::ratio<1, 1000000>>
        using namespace std::chrono;
        system_clock::time_point startClock = system_clock::now();
        std::this_thread::sleep_for(milliseconds(1000));
        system_clock::time_point endClock = system_clock::now();
        duration elapsed = endClock - startClock;
        auto miliseconds = duration_cast<milliseconds>(elapsed).count();
        std::cout << "time : " << miliseconds << std::endl;
    }

    std::future<bool> waitLoopResult = std::async(std::launch::deferred, wait_loop);
    std::cout << "waitLoopResult : " << waitLoopResult.get() << std::endl;
    
    {
        // c++ standard timeout
        using namespace std::chrono;
        auto duration = milliseconds(10);
        auto timePoint = system_clock::now() + milliseconds(10);

        {
            std::this_thread::sleep_for(duration); // duration
            std::this_thread::sleep_until(timePoint); // timePoint;
        }

        { 
            duration = milliseconds(1000);
            timePoint = system_clock::now() + duration;
            std::async(std::launch::async, conditionWaitFor, duration);
            std::async(std::launch::async, conditionWaitUntil, timePoint);

            wait_loop(); // std::cv_status::timeout, std::cv_status::no_timeout
        }

        {
            duration = milliseconds(100);
            std::timed_mutex m;
            m.try_lock_until(timePoint);

            std::recursive_timed_mutex rm;
            rm.try_lock_for(duration);
            rm.try_lock_until(timePoint);

            std::shared_timed_mutex sm;
            sm.try_lock_for(duration);
            sm.try_lock_until(timePoint);
        }

        {
            std::shared_timed_mutex sm;
            sm.try_lock_shared_for(duration);
            sm.try_lock_shared_until(timePoint);
        }

        {
            std::timed_mutex m;
            std::timed_mutex tm;
            std::unique_lock<std::timed_mutex> lk(tm, duration);
            std::cout << lk.owns_lock() << std::endl; // true, if lock was acquired.
            
            std::unique_lock<std::timed_mutex> lk2(tm, duration);
            std::cout << lk2.owns_lock() << std::endl; // true, if lock was acquired.

            std::unique_lock<std::timed_mutex> slk(tm, timePoint);
            std::cout << slk.try_lock_for(duration) << std::endl; // true, if lock was acquired.
            std::cout << slk.try_lock_until(timePoint) << std::endl; // true, if lock was acquired.
        }

        {
            std::shared_timed_mutex stm;
            std::shared_lock<std::shared_timed_mutex> lk(stm, duration);
            std::cout << lk.owns_lock() << std::endl; // true, if lock was acquired.
            std::shared_lock<std::shared_timed_mutex> lk2(stm, duration);
            std::cout << lk2.owns_lock() << std::endl; // true, if lock was acquired.
            std::shared_lock<std::shared_timed_mutex> lk3(stm, timePoint);
            std::cout << lk3.owns_lock() << std::endl; // true, if lock was acquired.
            
            {
                std::shared_timed_mutex _stm;
                std::shared_lock<std::shared_timed_mutex> lk4(_stm, std::defer_lock);
                std::cout << lk4.try_lock_for(duration) << std::endl; // true, if lock was acquired.
            }

            {
                std::shared_timed_mutex _stm;
                std::shared_lock<std::shared_timed_mutex> lk4(_stm, std::defer_lock);
                std::cout << lk4.try_lock_until(timePoint) << std::endl; // true, if lock was acquired.
            }
        }

        {
            duration = milliseconds(1000);
            timePoint = system_clock::now() + duration;
            {
                std::promise<std::string> p;
                std::future<std::string> data = p.get_future();
                std::thread promiseThread(worker, &p);
                data.wait_for(duration);
                //data.wait_until(timePoint);
                std::cout << data.get() << std::endl;
                promiseThread.join();
            }
            {
                std::promise<std::string> p;
                std::shared_future<std::string> data = p.get_future();
                std::thread promiseThread(worker, &p);
                //data.wait_for(duration);
                data.wait_until(timePoint); 
                // return std::future_status::timeout, 
                // return std::future_status::ready,
                // future_status::deferred 는 async()를 launch::deferred로 실행하였을 때
                // 즉, deferred function이 아직 시작되지 않았을 때!!
                // return std::future_status::deferred,
                std::cout << data.get() << std::endl;
                promiseThread.join();
            }
        }
    }

    // using syncronization of operations to simplify code
    // 스레드간에 데이터를 직접 공유하기보다 각각의 task가 필요한 데이터를 제공 받는 것이 더 좋음.
    // 그리고 그 결과는 그 결고가 필요한 다른 스레드들로 std::future를 사용하여 전파 된다.

    // Functional programming with futures
    //함수형 프로그래밍(functional programming, FP)이라는 용어는 함수 호출의 결과가 오직 
    // 그 함수에 전달된 파라미터에만 의존하고 외부 상태에 의존하지 않는 프로그래밍 스타일을 일컫는다. 
    // 이것은 수학적 함수 개념과 유사한데, 같은 파라미터로 2번 함수를 호출해도 그 결과는 동일하다는 것을 의미한다. 
    // 순수 함수(pure function)는 어떠한 외부 상태도 변경하지 않으며, 함수의 효과는 오직 리턴값으로만 제한된다

    // C++은 멀티패러다임 언어이어서, 프로그램을 FP 스타일로 작성하는 것이 가능하다. C++11에서는 이것이 더 쉬워졌는데, 
    // 람다 함수의 출현과 std::bind(), 그리고 타입을 추론하는 자동타입 때문이다. 
    // future는 C++에서 FP 스타일의 동시성을 만드는 마지막 퍼즐 조각이다. 
    // 계산 결과를 다른 스레드의 결과에 의존하게 해주기 위해 tuture를 스레드들간에 넘겨줄 수 있는데, 
    // 어떠한 공유 데이터도 명시적으로 접근할 필요가 없다.
    
    // FP-Style Quick Sort
    {
        std::list<int> l = {0, 4, 1, 2, 5, 3, 8, 6, 7, 9};
        auto result = sequential_quick_sort(l);

        for(auto n : result)
        {
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }

    // FP-Style Parallel Quick Sort
    {
        std::list<int> l = {0, 4, 1, 2, 5, 3, 8, 6, 7, 9};
        auto result = pararrel_quick_sort(l);

        for(auto n : result)
        {
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }

    // synchronizing operations with message passing
    // 메시지 전달로 동작을 동기화하기
    // ​CSP(Communicating Sequential Processes)의 개념은 간단하다. 
    // 공유 데이터가 없다면, 각각의 스레드들은 완전히 독립적으로 동작할 수 있으며 
    // 오직 그 스레드가 받은 메시지에 반응하기만 하면 된다는 것이다. 
    // 그러므로 각각의 스레드는 효과적으로 상태기계(state machine)가 될 수 있는데, 
    // 메시지를 받으면 현재 상태에 따른 동작을 수행하고 일정한 방법으로 그 상태를 업데이트하고
    // 다른 스레드에 하나 이상의 메시지를 전달할 수도 있다.

    {
        // sample implementation of spawn_task
        std::future<int> result = spawn_task([](int x)
        { 
            std::cout << x << std::endl;
            return x; 
        }, 100);

        result.get();
    }

    // Continuation-style concurrency with the concurrency TS(Technical specifications)
    // CSP : 공유하는 data가 없다면, thread는 전체적으로 독립적으로 응답될 수 있고
    // 전적으로 thread가 message를 받았을 때, 응답에 있어서 어떻게 동작해는지.
    // -> 각각의 thread는 state machine이다.
    // basic_state_machine.h 참조

    // while을 통해서 무한히 switch문을 반복합니다.
    // nState가 각각의 상태를 뜻하고, switch문에서 그 상태값에 따라서 정해진 작업을 합니다.
    // 각각의 작업이 마친 후 다음에 실행할 상태값으로 바꿔줍니다.

    // State가 여러 단계로 나눠질 경우 이런 식의 while-switch 구문이 여러 단계로 중첩이 됩니다.
    // State도 nCurrentState, nNextState 등으로 나누어서 값을 넣어서 현재 상태와 다음 상태의 값을 보고 각각의 작업을 다르게 하는 식으로 구현을 하기도 합니다.
    // State Machine의 가장 큰 장점은 Diagram으로 쉽게 설계가 가능하며, diagram만 보고도 쉽게 구현 할 수 있습니다.
    
    // A simple implementation of an ATM logic class
    /*
        atm_machine.h 참조
    */

    // chaining continuations
    // waiting for more than one future
    // waiting for the first future in a set with when-any
    // Latches and barriers in the concurrency TS
    // a basic latch type: std::experimental::latch
    // experimental::barrier: a basic barrier
    // std::experimental::flex_barrier-std::experimental::barrier's flexible friend

    return 0;
}