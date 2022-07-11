#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>
#include <list>
#include <vector>
#include "threadsafe_stack.h"

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
    // - ???
    // 4) lock 계층을 사용하라
    // 5) 이 guide line을 잠금을 넘어서 확장해라.

    return 0;
}