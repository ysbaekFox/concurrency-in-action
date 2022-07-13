#ifndef HIERACHICAL_MUTEX
#define HIERACHICAL_MUTEX

#include <mutex>
#include <iostream>

class hierarchical_mutex
{
public:
    explicit hierarchical_mutex(unsigned long value)
        : hierarchical_value(value),
          previous_hierarchical_value(0)
    {

    }

    void lock()
    {
        check_for_hierarchical_violation();
        internal_mutex.lock();
        update_hierarchical_value();
    }

    void unlock()
    {
        if(this_thread_hierarchical_value != hierarchical_value)
        {
            throw std::logic_error("[unlock] mutex hierarchical violated");
        }
        this_thread_hierarchical_value = previous_hierarchical_value;
        internal_mutex.unlock();
    }

    bool try_lock()
    {
        check_for_hierarchical_violation();
        if(!internal_mutex.try_lock())
        {
            std::cout << "already locked" << std::endl;
            return false;
        }
        update_hierarchical_value();
        return true;
    }


private:
    std::mutex internal_mutex;
    unsigned long const hierarchical_value;
    unsigned long previous_hierarchical_value;

    // thread_local : thread 별로 고유한 저장공간을 가질 수 있는 방법
    static thread_local unsigned long this_thread_hierarchical_value;

    void check_for_hierarchical_violation()
    {
        if(this_thread_hierarchical_value <= hierarchical_value)
        {
            throw std::logic_error("[check] mutex hierarchical violated");
        }
    }

    void update_hierarchical_value()
    {
        previous_hierarchical_value = this_thread_hierarchical_value;
        this_thread_hierarchical_value = hierarchical_value;
    }
};

thread_local unsigned long hierarchical_mutex::this_thread_hierarchical_value(ULONG_MAX);

#endif