#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

class spinlock_mutex
{
	// spin lock
	// 락을 걸지 못하면 무한 루프를 돌면서 계속 얻으려고 시도하는 동기화 기법

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
	// 실제로는, a = b + 1 부분의 실행이 끝나기 전에
	// b = 1 이 먼저 실행이 끝나게 됨.
	// foo 함수의 입장에선 크게 문제는 없습니다.
	a = b + 1;
	b = 1;

	// 현재 a = 0, b = 0;
	// a = 1;  // 캐시에 없음
	// b = 1;  // 캐시에 있음
	// 위와 같은 상황에서 다른 스레드에서 a와 b에 접근하는데..
	// b는 1인데, a는 0인 상황이 생길 수 있습니다!!!
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
  // data 가 준비될 때 까지 기다린다.
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

	// 참고로 atomic template을 사용하는 타입은
	// trivial 복사-할당 연산자를 가져아만 합니다. 
	// (어떠한 가상함수 혹은 가상 기반 클래스도 가져서는 안되고, 컴파일러가 생성하는 복사 할당 연산자를 가져야 함.)

	foo();
	std::cout << a << std::endl;
	std::cout << b << std::endl;

	// 원자적 연산이란, CPU 가 명령어 1 개로 처리하는 명령으로, 중간에 다른 쓰레드가 끼어들 여지가 전혀 없는 연산을 말함.
	// 즉, 이 연산을 반 정도 했다 는 있을 수 없고 이 연산을 했다 혹은 안 했다 만 존재할 수 있음.
	// 마치 원자처럼 쪼갤 수 없다 해서 원자적(atomic) 이라고 함. 또한 뮤텍스도 필요하지 않음.
	
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

	// atomic 객체들의 경우 원자적 연산 시에 메모리에 접근할 때
	// 어떠한 방식으로 접근하는지 지정할 수 있습니다.

	// memory_order_relaxed
	// - cpu에게 가장 자유로운 권한을 주어서 실행 순서가 바뀔 수 있음 (단 원자적인 것은 여전함.)
	// - 그래서 위의 counter 같이 제한적인 상황에서만 사용 가능

	// memory_order_relaxed를 사용할 경우
	// 아래 예제에서 is_ready 값이 true가 됐을 때
	// data 값은 10이 아닐 수도 있습니다.!!!
	//
	// memory_order_release
	// - 해당 명령 이전의 모든 메모리 명령들이 명령 이후로 재배치 되는 것을 금지!!
	// memory_oreder_acquire
	// - 이 옵션으로 읽는 쓰레드가 있다면, memory_order_release 이전에 오는 모든 메모리
	// 먕량들이 해당 쓰레드에 의해서 관찰 될 수 있어야 한다.
	// -> is_ready(store) 밑으로 *data = 10이 올 수 없게 됨.
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

	// release 와 acquire의 사용 예.
	// relax 명령어끼리는 순서가 바뀔 수 있지만, release 이후로 재배치 될 순 없다.
	std::thread _t1(_producer);
	std::thread _t2(_consumer);

	_t1.join();
	_t2.join();

	// 이외
	// memory_order_acq_rel
	// - acquire와 release를 모두 수행하는 방식.
	// memory_order_seq_cst
	// - 메모리 명령 재배치도 없고, 모든 스레드에서 모든 시점에 동일한 값을 관찰할 수 있는 방식

	bool expected = true;
	// atomic은 비원자적 타입에서 확장 가능함.
	std::atomic<bool> b(true); // b = true 이런식으로도 가능

	// 현재 값에 의존하여, 혹은 의존하지 않고 새로운 값을 저장하기.
	// 원자적 변수(b)를 제공된 예상값(expected)와 비교하여 만약 그들이 같다면 제공된 목적값(false or true)를 저장한다.
	// compare_exchange_weak 는 같을 경우 목적값을 저장하고 1을 반환
	// compare_exchange_strong 도 같을 경우 목적값을 저장하고 1을 반환
	// 단 weak와 strong의 차이점은 weak는 원래 값과 기대 값이 일치하더라도 저장을 수행하지 않을 수 있음
	// 이때 값은 변경되지 않고 false를 반환하므로 보통은 루프안에서 사용되어야 합니다. 
	// (이것은 시스템에 의해 스레드 명령어들이 다른 스레드로 스케쥴링 되면 발생할 수 있는 거짓된 실패 때문.....)

	// 또한, compare_exchange_weak와 compare_exchange_strong는
	// 2개의 메모리 순서 파라미터를 가질 수 있다. 이것은 성공과 실패의 경우에 따라 다른 메모리 순서를 가질 수 있게 하는 것이다.
	// ex) b.compare_exchange_weak(expected, false, std::memory_order_relaxed, std::memory_order_release)

	std::cout << b.compare_exchange_weak(expected, false) << std::endl;
	std::cout << b << std::endl;

	bool expected2 = false;
	std::cout << b.compare_exchange_strong(expected2, true) << std::endl;
	std::cout << b << std::endl;

	// thread1은 store release하기 전에 relaxed로 data array에 값 셋팅 (relax는 release 이후로 재배치 불가)
	// thread2는 thread1에서 store가 되면, thread3에서 acquire loop의 값을 true로 store
	// thread3는 thread2에서 store할 때까지 기다리는 load 호출 loop를 가지고 thread1의 data에 접근함.

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

	{ // thread1과 thread2를 통합하도록 개선
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