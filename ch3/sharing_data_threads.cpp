#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>
#include <list>
#include <vector>
#include <exception>

#include "threadsafe_stack.h"
#include "hierarchical_mutex.h"

// 이번 챕터는 thread간의 안전한 data 공유 방법에 관한 것 임.
// (잠재적인 위험은 피하고, benefit은 최대로 하는 것 !!)

// Race Condition
// 두 개 이상의 프로세스가 공통 자원을 병행적으로(concurrently) 읽거나 쓰는 동작을 할 때, 
// 공용 데이터에 대한 접근이 어떤 순서에 따라 이루어졌는지에 따라 그 실행 결과가 같지 않고 달라지는 상황
// 1) Mutual exclusion
// 2) Deadlock
// 3) Starvation (기아)

std::list<int> some_list;
std::mutex some_mutex;

void add_to_list(int new_value)
{
    std::lock_guard<std::mutex> guard(some_mutex);
    some_list.push_back(new_value);
}

bool list_contains(int value_to_find)
{
    std::lock_guard<std::mutex> guard(some_mutex);
    return std::find(some_list.begin(), some_list.end(), value_to_find) \
                != some_list.end();
}

class some_data
{
public:
    void do_something()
    {
    }
private:
    int a;
    std::string b;
};

class data_wrapper
{
public:
    template<typename Function>
    void process_data(Function func)
    {
        std::lock_guard<std::mutex> guard(m);
        func(data);
    }

private:
    some_data data;
    std::mutex m;
};

some_data* unprotected;
void malicious_function(some_data& protected_data)
{
    unprotected = &protected_data;
}

data_wrapper x;
void foo()
{
    x.process_data(malicious_function);
    unprotected->do_something();
}

class some_big_object
{ };

void swap(some_big_object& lhs, some_big_object& rhs)
{
    some_big_object tmp(std::move(lhs));
    lhs = std::move(rhs);
    rhs = std::move(tmp);
}

class X
{
public:
    X(some_big_object const& sd)
        : some_detail(sd)
    {
    }

    friend void swap(X& lhs, X& rhs)
    {
        if(&lhs == &rhs)
            return;
        std::lock(lhs.m, rhs.m); // 동시에 lock
        // adopt_lock 사용하여, lock_guard 씌우기.
        std::lock_guard<std::mutex> lock_a(lhs.m, std::adopt_lock);
        std::lock_guard<std::mutex> lock_b(rhs.m, std::adopt_lock);
        swap(lhs.some_detail, rhs.some_detail);
    }

private:
    some_big_object some_detail;
    std::mutex m;
};

hierarchical_mutex high_level_mutex(10000);
hierarchical_mutex low_level_mutex(5000);
hierarchical_mutex other_mutex(6000);

int low_level_func()
{
    std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
    return 512;
}

void high_level_func()
{
    std::lock_guard<hierarchical_mutex> lk(high_level_mutex);
    std::cout << "high_level_stuff : " << low_level_func() << std::endl;
}

void other_stuff()
{
    high_level_func();
    std::cout << "do other stuff" << std::endl;
}

void thread_a()
{
    high_level_func();
}

void thread_b()
{
    std::lock_guard<hierarchical_mutex> lk(other_mutex);
    other_stuff();
}

std::unique_lock<std::mutex> get_lock()
{
    static std::mutex some_mutex;
    std::unique_lock<std::mutex> lk(some_mutex);
    std::cout << &lk << std::endl;
    return lk; // mutex 소유권 넘어감.
}

void process_data()
{
    // get_lock으로부터 mutex 소유권 넘어옴.
    std::unique_lock<std::mutex> lk(get_lock());
    std::cout << &lk << std::endl;
    std::cout << "do something" << std::endl;
}

void get_and_process_data()
{
    static int data = 0;
    std::mutex m;
    std::unique_lock<std::mutex> my_lock(m, std::defer_lock);
    my_lock.lock(); // 필요한 부분에서만 lock 수행
    data = std::rand();
    std::cout << data << std::endl;
    my_lock.unlock();
}

class Y
{
public:
    Y(int sd) : some_detail(sd)
    {
        
    }

    friend bool operator==(Y const& lhs, Y const& rhs)
    {
        if(&lhs == &rhs)
            return true;
        int const lhs_value = lhs.get_detail();
        int const rhs_value = rhs.get_detail();
        return lhs_value == rhs_value;
    }

private:
    int some_detail;
    mutable std::mutex m; // mutable : const 내부에서 값 수정이 필요할 때 사용

    int get_detail() const
    {
        std::lock_guard<std::mutex> lock_a(m);
        return some_detail;
    }
};


int main()
{
    // mutex를 사용하여 공유 데이터 보호할 수 있음
    // 하지만, 항상 묘책(Silver Bullet)은 아님, DeadLock 가능성이 있음.
    // std::mutex를 바로 사용해도 되지만, std::lock_guard로 한번 래핑해서 사용할 수 있음.

    add_to_list(0);
    add_to_list(1);
    add_to_list(2);
    std::cout << "0 is exist : " << list_contains(0) << std::endl;
    std::cout << "1 is exist : " << list_contains(1) << std::endl;
    std::cout << "2 is exist : " << list_contains(2) << std::endl;
    std::cout << "3 is exist : " << list_contains(3) << std::endl;

    // 공유 데이터 보호를 위한 Structing Code
    // foo 실행 시, 완벽하게 안전할까?
    // 그렇지 않다, do_something() 동작은 mutex lock 없이 동작하기 때문이다.
    // 이것은 C++ Lib의 도움을 받을 수 있는 영역이 아니고, 사용자가 잘 처리해야한다.
    // 팁을 주자면
    // 보호되지 않은 data의 pointer, reference를 lock scope 밖으로 전달하지마라
    // (함수의 return으로서, user가 제공한 함수의 arguments로서, 외부에서 접근 가능한 메모리(변수)에 저장함으로서)
    foo();

    // 선천적으로 interface에서 발견되는 race condition
    // stack의 경우 push, pop, empty, top, size 5개의 동작이 가능하다.
    // 각각의 interface가 모두 보호되고 있어도 race condition이 발생할 수 있다.
    // (46p Table 3.1 참고)

    // 어떻게하면 race condition을 막을 수 있을까??

    // option 1) 아래와 같이 reference로 동작하게 하는 것  
    // std::vector<int> result;
    // some_stack.pop(result);
    // 이 방법은 비용 측면에서 단점이 있다.

    // option 2) move 또는 no throw ctor 사용. (매우 제한적이라 별로)
    // option 3) pop item을 pointer로 return하는 방법
    // -> 비용적 장점이 있지만, 메모리 관리의 단점이 있다.
    // -> shared_ptr을 사용하면 좋겠지?
    // option 4) option 1과, 2 또는 3을 같이 제공하는 것 (Good)

    // option 1, 3 같이 쓴 예
    // 하지만, global single mutex로 동작하면 성능이 안좋음
    threadsafe_stack<int> threadStack;

    threadStack.push(0);
    threadStack.push(1);
    threadStack.push(2);
    threadStack.push(3);

    std::cout << *(threadStack.pop()) << std::endl;
    std::cout << *(threadStack.pop()) << std::endl;
    std::cout << *(threadStack.pop()) << std::endl;
    std::cout << *(threadStack.pop()) << std::endl;

    // dead lock
    X a(some_big_object{});
    X b(some_big_object{});
    swap(a, b);

    // lock 없이도 dead lock 발생할 수 있다.
    // (2개 이상의 thread가 다른 thread의 완료를 대기할 때)
    // 1) 중첩된 lock을 사용하지마라
    // 2) 잠금을 유지하는 동안, 사용자가 제공한 코드를 호출하지 마라
    // - 사용자가 제공한 코드는 중첩된 lock을 사용할 수 도 있고, 무엇을 할지 알 수 없다.
    // 3) lock이 여러개 필요하면, 순서를 고정 시켜라. (3.2.4에서 방법 제시)
    // - 첫번째 케이스는 사용자가에게 그 짐을 전가하는 것 (예를 들어 stack처럼), 
    //   이러한 것이 발생할 때 꽤나 분명하다, 그래서 그 짐을 넘기는 것이 특별히 어렵지 않다.
    // - 다른 나머지의 경우 예를 들어 list에서 node를 제거하는 것과 같은 경우는
    //   순서를 반드시 고정 시켜야 할 것 이다.

    // 4) lock 계층을 사용하라
    std::thread hierarchicalMutexThread(thread_a);
    hierarchicalMutexThread.join();

    // high lock 전에 other lock 실행 되어 error throw 됨.
    // 순서를 강제하는 deadlock 방지 기법!!
    //std::thread hierarchicalMutexThread2(thread_b);
    //hierarchicalMutexThread2.join();

    // 5) 그리고 이 guide line들을 잠금을 넘어서 확장해라.

    // std::unique_lock
    // std::unique_lock은 std::lock_guard보다 좀 더 유연함을 제공한다.
    // - std::unique_lock은 lock / unlock 할 수 있음.
    // - std::lock_guard는 생성자와 소멸자를 통해서만 lock 가능함.
    
    // adopt_lock : 이미 호출한 스레드가 락을 소유 했고 스레드에서 자체적으로 락을 관리한다고 가정합니다.  
    // defer_lock : 뮤텍스 객체를 저장만하고 락을 시도하지 않습니다. 락의 시점을 지연시켜 사용자가 필요할 때 호출 할 수 있습니다.
    // try_to_lock : 잠금하지 않고 뮤텍스의 소유권을 얻으려고 시도합니다.

    // unique_lock을 사용한 동시 잠금 방법. (생성자 사용 X)
    std::mutex lm;
    std::mutex rm;

    std::unique_lock<std::mutex> lock_a(lm, std::defer_lock);
    std::unique_lock<std::mutex> lock_b(rm, std::defer_lock);
    std::lock(lock_a, lock_b);

    // mutex 소유권 전달.
    // std::move를 사용하여 명시적으로 사용하거나,
    // 함수로부터 instance가 return 될 때, 자동으로 발생한다.
    process_data();

    // Locking at an appropriate granularity (적절한 낱알에서 잠그기?)
    get_and_process_data();

    // Locking one mutex at a time in a comparison operator
    Y y_a(10);
    Y y_b(10);
    std::cout << "y_a and y_b is ame : " << (y_a == y_b) << std::endl;

    // protecting a data structure with std::shared_mutex.
    
    // recursive locking

    return 0;
}