#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

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

int a = 0;
int b = 0;


void worker(std::atomic<int>& counter) 
{
  for (int i = 0; i < 10000; i++) 
  {
    counter++;
  }
}

void foo()
{
	// �����δ�, a = b + 1 �κ��� ������ ������ ����
	// b = 1 �� ���� ������ ������ ��.
	// foo �Լ��� ���忡�� ũ�� ������ �����ϴ�.
	a = b + 1;
	b = 1;

	// ���� a = 0, b = 0;
	// a = 1;  // ĳ�ÿ� ����
	// b = 1;  // ĳ�ÿ� ����
	// ���� ���� ��Ȳ���� �ٸ� �����忡�� a�� b�� �����ϴµ�..
	// b�� 1�ε�, a�� 0�� ��Ȳ�� ���� �� �ֽ��ϴ�!!!
}

std::atomic<bool> is_ready;
std::atomic<int> data[3];

void _producer() 
{
  data[0].store(1, std::memory_order_relaxed);
  data[1].store(2, std::memory_order_relaxed);
  data[2].store(3, std::memory_order_relaxed);
  is_ready.store(true, std::memory_order_release);
}

void _consumer() {
  // data �� �غ�� �� ���� ��ٸ���.
  while (!is_ready.load(std::memory_order_acquire)) 
  {
	// do something
  }

  std::cout << "data[0] : " << data[0].load(std::memory_order_relaxed) << std::endl;
  std::cout << "data[1] : " << data[1].load(std::memory_order_relaxed) << std::endl;
  std::cout << "data[2] : " << data[2].load(std::memory_order_relaxed) << std::endl;
}

int main()
{
	std::thread t1([]() { spinLock.lock(); });
	std::thread t2([]() { spinLock.lock(); });
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	spinLock.unlock();
	t1.join();
	t2.join();

	// ����� atomic template�� ����ϴ� Ÿ����
	// trivial ����-�Ҵ� �����ڸ� �����Ƹ� �մϴ�. 
	// (��� �����Լ� Ȥ�� ���� ��� Ŭ������ �������� �ȵǰ�, �����Ϸ��� �����ϴ� ���� �Ҵ� �����ڸ� ������ ��.)

	foo();
	std::cout << a << std::endl;
	std::cout << b << std::endl;

	// ������ �����̶�, CPU �� ��ɾ� 1 ���� ó���ϴ� �������, �߰��� �ٸ� �����尡 ����� ������ ���� ���� ������ ����.
	// ��, �� ������ �� ���� �ߴ� �� ���� �� ���� �� ������ �ߴ� Ȥ�� �� �ߴ� �� ������ �� ����.
	// ��ġ ����ó�� �ɰ� �� ���� �ؼ� ������(atomic) �̶�� ��. ���� ���ؽ��� �ʿ����� ����.
	
	std::atomic<int> counter = 0;
	std::vector<std::thread> workers;
	for (int i = 0; i < 4; i++) 
	{
		workers.push_back(std::thread(worker, std::ref(counter)));
	}

	for (int i = 0; i < 4; i++) 
	{
		workers[i].join();
	}

	std::cout << "Counter : " << counter << std::endl;

	// atomic ��ü���� ��� ������ ���� �ÿ� �޸𸮿� ������ ��
	// ��� ������� �����ϴ��� ������ �� �ֽ��ϴ�.

	// memory_order_relaxed
	// - cpu���� ���� �����ο� ������ �־ ���� ������ �ٲ� �� ���� (�� �������� ���� ������.)
	// - �׷��� ���� counter ���� �������� ��Ȳ������ ��� ����

	// memory_order_relaxed�� ����� ���
	// �Ʒ� �������� is_ready ���� true�� ���� ��
	// data ���� 10�� �ƴ� ���� �ֽ��ϴ�.!!!
	//
	// memory_order_release
	// - �ش� ��� ������ ��� �޸� ��ɵ��� ��� ���ķ� ���ġ �Ǵ� ���� ����!!
	// memory_oreder_acquire
	// - �� �ɼ����� �д� �����尡 �ִٸ�, memory_order_release ������ ���� ��� �޸�
	// �ҷ����� �ش� �����忡 ���ؼ� ���� �� �� �־�� �Ѵ�.
	// -> is_ready(store) ������ *data = 10�� �� �� ���� ��.
	std::atomic<bool> is_ready(false);
	int data = 0;
	
	std::thread consumer([](std::atomic<bool>* is_ready, int* data)
	{
		while(!is_ready->load(std::memory_order_relaxed))
		{ 
			// do something
		}

		std::cout << "Data : " << *data << std::endl;
	}, &is_ready, &data);

	std::thread producer([](std::atomic<bool>* is_ready, int* data)
	{
		*data = 10;
		is_ready->store(true, std::memory_order_relaxed);
	}, &is_ready, &data);

	producer.join();
	consumer.join();

	// release �� acquire�� ��� ��.
	// relax ��ɾ���� ������ �ٲ� �� ������, release ���ķ� ���ġ �� �� ����.
	std::thread _t1(_producer);
	std::thread _t2(_consumer);

	_t1.join();
	_t2.join();

	// �̿�
	// memory_order_acq_rel
	// - acquire�� release�� ��� �����ϴ� ���.
	// memory_order_seq_cst
	// - �޸� ��� ���ġ�� ����, ��� �����忡�� ��� ������ ������ ���� ������ �� �ִ� ���

	bool expected = true;
	// atomic�� ������� Ÿ�Կ��� Ȯ�� ������.
	std::atomic<bool> b(true); // b = true �̷������ε� ����

	// ���� ���� �����Ͽ�, Ȥ�� �������� �ʰ� ���ο� ���� �����ϱ�.
	// ������ ����(b)�� ������ ����(expected)�� ���Ͽ� ���� �׵��� ���ٸ� ������ ������(false or true)�� �����Ѵ�.
	// compare_exchange_weak �� ���� ��� �������� �����ϰ� 1�� ��ȯ
	// compare_exchange_strong �� ���� ��� �������� �����ϰ� 1�� ��ȯ
	// �� weak�� strong�� �������� weak�� ���� ���� ��� ���� ��ġ�ϴ��� ������ �������� ���� �� ����
	// �̶� ���� ������� �ʰ� false�� ��ȯ�ϹǷ� ������ �����ȿ��� ���Ǿ�� �մϴ�. 
	// (�̰��� �ý��ۿ� ���� ������ ��ɾ���� �ٸ� ������� �����층 �Ǹ� �߻��� �� �ִ� ������ ���� ����.....)

	// ����, compare_exchange_weak�� compare_exchange_strong��
	// 2���� �޸� ���� �Ķ���͸� ���� �� �ִ�. �̰��� ������ ������ ��쿡 ���� �ٸ� �޸� ������ ���� �� �ְ� �ϴ� ���̴�.
	// ex) b.compare_exchange_weak(expected, false, std::memory_order_relaxed, std::memory_order_release)

	std::cout << b.compare_exchange_weak(expected, false) << std::endl;
	std::cout << b << std::endl;

	bool expected2 = false;
	std::cout << b.compare_exchange_strong(expected2, true) << std::endl;
	std::cout << b << std::endl;

	// thread1�� store release�ϱ� ���� relaxed�� data array�� �� ���� (relax�� release ���ķ� ���ġ �Ұ�)
	// thread2�� thread1���� store�� �Ǹ�, thread3���� acquire loop�� ���� true�� store
	// thread3�� thread2���� store�� ������ ��ٸ��� load ȣ�� loop�� ������ thread1�� data�� ������.

	{
		std::atomic<int> data[5];
		std::atomic<bool> sync1(false), sync2(false);
		
		std::thread thread1([&data, &sync1]()
		{
			data[0].store(42, std::memory_order_relaxed);
			data[1].store(97, std::memory_order_relaxed);
			data[2].store(17, std::memory_order_relaxed);
			data[3].store(-141, std::memory_order_relaxed);
			data[4].store(2003, std::memory_order_relaxed);
			sync1.store(true, std::memory_order_release);
		});

		std::thread thread2([&sync1, &sync2]()
		{
    		while (!sync1.load(std::memory_order_acquire));
    			sync2.store(true, std::memory_order_release);
		});

		std::thread thread3([&sync2, &data]()
		{
			while (!sync2.load(std::memory_order_acquire)) {}

			assert(data[0].load(std::memory_order_relaxed) == 42);
			assert(data[1].load(std::memory_order_relaxed) == 97);
			assert(data[2].load(std::memory_order_relaxed) == 17);
			assert(data[3].load(std::memory_order_relaxed) == -141);
			assert(data[4].load(std::memory_order_relaxed) == 2003);
		});

		thread1.join();
		thread2.join();
		thread3.join();
	}

	{ // thread1�� thread2�� �����ϵ��� ����
		std::atomic<int> data[5];
		std::atomic<int> sync(false);
		
		std::thread thread1([&data, &sync]()
		{
			data[0].store(42, std::memory_order_relaxed);
			data[1].store(97, std::memory_order_relaxed);
			data[2].store(17, std::memory_order_relaxed);
			data[3].store(-141, std::memory_order_relaxed);
			data[4].store(2003, std::memory_order_relaxed);
			sync.store(1, std::memory_order_release);
		});

		std::thread thread2([&sync]()
		{
			int expected = 1;
			while(!sync.compare_exchange_strong(expected, 2, std::memory_order_acq_rel))
			{ }
		});

		std::thread thread3([&sync, &data]()
		{
			while (sync.load(std::memory_order_acquire) < 2) {}

			assert(data[0].load(std::memory_order_relaxed) == 42);
			assert(data[1].load(std::memory_order_relaxed) == 97);
			assert(data[2].load(std::memory_order_relaxed) == 17);
			assert(data[3].load(std::memory_order_relaxed) == -141);
			assert(data[4].load(std::memory_order_relaxed) == 2003);
		});

		thread1.join();
		thread2.join();
		thread3.join();
	}

	return 0;
}