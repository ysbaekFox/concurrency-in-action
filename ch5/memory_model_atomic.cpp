#include <iostream>
#include <atomic>
#include <thread>

class spinlock_mutex
{
	// spin lock
	// ���� ���� ���ϸ� ���� ������ ���鼭 ��� �������� �õ��ϴ� ����ȭ ���

	std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
	spinlock_mutex()
	{

	}
	void lock()
	{
		// in C++20
		// test is retun value -> flag.test()
		while (flag.test_and_set(std::memory_order_acquire));
		std::cout << "lock done" << std::endl;
	}
	void unlock()
	{
		flag.clear(std::memory_order_release);
		std::cout << "unlock done" << std::endl;
	}
};

spinlock_mutex spinLock;
std::atomic_flag testFlag;

std::atomic<bool> flag(true);

int main()
{
	// memory_order_relaxed ?
	// memory_order_acquire ? 
	// memory_order_release ?

	std::thread t1([]() { spinLock.lock(); });
	std::thread t2([]() { spinLock.lock(); });
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	spinLock.unlock();
	t1.join();
	t2.join();



	return 0;
}