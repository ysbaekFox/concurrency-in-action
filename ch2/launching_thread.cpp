#include <thread>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <numeric>

class background_task
{
public:
	void do_something() const;
	void do_something_else() const;

	void operator() () const 
	{
		do_something();
		do_something_else();
	}
};

void background_task::do_something() const
{
	std::cout << "do something" << std::endl;
}

void background_task::do_something_else() const
{
	std::cout << "do something else" << std::endl;
}

struct func
{
	func(int& _i)
		: i(_i)
	{

	}

	void operator() ()
	{
		for (int j = 0; j < 100000; j++)
		{
			// do_something 함수 내부에서
			// reference i에 접근 중
			do_something();
		}

		std::cout << "func done" << std::endl;
	}

	void do_something()
	{
		i++;
	}

	int& i;
};

void oops()
{
	int localVariable = 0;
	func my_func(localVariable);
	std::thread my_thread(my_func);
	my_thread.detach();

	// function return 되면
	// localVriable 삭제되고, thread는 남아있어서 dangling 발생.
}

void copyFunc(int i, std::string const& s)
{
	std::cout << "i : " << i << ", s : " << s.c_str() << std::endl;
}

void refFunc(int i, std::string& s)
{
	std::cout << "i : " << i << ", s : " << s.c_str() << std::endl;
}

class X
{
public:
	void do_something()
	{
		std::cout << "do something" << std::endl;
	}
};

using big_object = std::string;

void process_big_object(std::unique_ptr<big_object> bObj)
{
	std::cout << bObj.get() << std::endl;
}

void some_function()
{
	std::cout << "some_function" << std::endl;
}

void some_other_function()
{
	std::cout << "some_other_function" << std::endl;
}

class scoped_thread
{
public:
	scoped_thread(std::thread _t)
		: t(std::move(_t))
	{
		if(!t.joinable())
		{
		std::cout << "throw no thread" << std::endl;
			throw std::logic_error("No Thread");
		}
	}

	~scoped_thread()
	{
		t.join();
		std::cout << "~scoped_thread" << std::endl;
	}

	scoped_thread(scoped_thread const &) = delete;
	scoped_thread& operator=(scoped_thread const&) = delete;
	
private:
	std::thread t;
};

int main()
{
	background_task f;
	std::thread my_thread(f);
	my_thread.join();


	/*
	* 주의 : most vexing parse!!
	* 아래 코드는 동작하지 않습니다.
	*/

	std::thread my_thread2(background_task());
	// my_thread2.join(); // Error !!

	/*
	* most vexing parse 회피 방법 2가지
	*/
	std::thread my_thread3((background_task()));
	my_thread3.join();

	std::thread my_thread4((background_task()));
	my_thread4.join();

	/*
	* Lambda 사용하기
	*/

	std::thread lambda_thread([]() { std::cout << "thread joined" << std::endl; });
	lambda_thread.join();

	/*
	* 일단, thread를 시작하면 thread가 finish 될 때까지 기다릴지 (join)
	* 또는 구동 중인 thread를 leave 해야할지를 결정해야 한다. (detach)
	* 만약 2가지를 결정하기 전에 thread가 destory 된다면, program은 std::terminate()를 호출한다.
	*/

	std::thread lambda_thread2([]() { std::cout << "thread detached" << std::endl; });
	lambda_thread2.detach(); // detach 하면, thread 동작 보장 안 됨.

	/*
	* 만약 생성한 thread를 wait하지 않는다면
	* thread에 의해 접근 되는 data는 thread가 finish 될때 까지 유효성을 보장해야만 한다.
	* 그렇지 않으면 thread가 object에 접근 한 후, object가 destory 될 경우 undefined behavior가 발생한다.
	* (이것은 사실 새로운 이슈가 아니다, single thread 환경에서도 문제이다.)
	*/

	/*
	* 하지만, thread들의 사용은, 직면하는 생명 주기(lifetime) 이슈들에 대해
	* 추가적인 기회를 제공한다.
	*/

	/*
	 * thread가 여전히 지역 변수에 접근하는 동안, function가 return하는 상황
	 * (아래 oops는 dangling 이슈 발생.)
	 */

	// 이 Scenario를 처리하는 방법은 다음과 같다.
	// 1) sharing data를 사용하지 않고 copy한 data를 사용하는 것 (Bad Idea)
	// 2) join 사용
	//    - 단순 join 사용
	//    - RAII 패턴을 사용하는 thread_guard Wrapper 사용

	// undefined behavior!!!!!!
	// oops();

	// detach를 호출하는 것은 thread를 background로 보내고
	// 직접적인 통신이 없다는 의미이다. 더 이상 thread가 완료 될 때까지 기다리는 것이 불가능하다는 것이다. (join 불가능)
	// detach thread는 daemon thread(in UNIX)라고도 불린다. user와 명시적 interface 없이 background에서 동작한다.
	// file-system을 모니터링하거나, clearing 동작을 수행하거나, 캐시 관리 혹은 data structure 최적화 등을 수행한다.

	// detach thread를 사용하는 것은 thread가 완료되었을 때를 식별하기 위한 다른 메커니즘에 사용되거나
	// fire-and-forget task 방식에 사용하는 것은 타당할 것 같습니다.

	/* detach 활용 예.
	void edit_document(std::string const& filename)
	{
		open_document_and_display_gui(filename);

		while (!done.editing())
		{
			user_command cmd = get_user_input();

			if (cmd.type == open_new_document)
			{
				// 만약 워드를 추가로 open하면
				// 동일한 thread를 detach만 해주면 된다!!!.
				std::string const new_name = get_filename_from_user();
				std::thread t(edit_document, new_name);
				t.detach();
			}
			else
			{
				process_user_input(cmd);
			}
		}
	}
	*/

	/*
	* thread function에 parameter 전달하기.
	*/

	// 아래 동작은 실제로는 컴파일러에 의해 Copy가 발생함.
	std::thread copyThread(copyFunc, 1, "hello");
	copyThread.join();

	// 아래와 같이 사용해야 함.
	// void update_data_for_widget(widget_id w, widget_data& data);
	// std::thread t(update_data_for_widget, w, data);

	std::string refStr = "ref";
	//std::thread refThread(refFunc, 2, refStr); //  얘는 빌드 에러 남.
	std::thread refThread(refFunc, 3, std::ref(refStr)); // 이렇게 써야 함
	refThread.join();

	//std::bind와 사용 방법이 같아서, 아래처럼도 사용 가능.
	// 클래스 내부에서는 아래처럼도 사용 가능.
	// std::thread bindThread(&X::do_something, this);
	// 코드에서 my_x.do_something()이 실행 될 것임.
	X my_x;
	std::thread bindThread(&X::do_something, &my_x);
	bindThread.join();

	// 이번엔 복사는 할 수 없고 move만 가능한 object에 대한 Case 임.
	std::unique_ptr<big_object> p = std::make_unique<big_object>("big_object");
	std::cout << p << std::endl;
	std::thread moveThread(process_big_object, std::move(p));
	moveThread.join();

	// thread의 소유권 이동
	std::thread transferring1(some_function);
	std::thread transferring2 = std::move(transferring1);
	transferring1 = std::thread(some_other_function);

	std::thread transferring3;
	transferring3 = std::move(transferring2);
	
	// transferring1은 이미 소유권을 가지고 있음.
	// std::terminate() 발생
	// transferring1 = std::move(transferring3);

	transferring1.join();
	transferring3.join();

	// move의 장점은
	// thread_guard에 std::move를 적용할 수 있다는 것이다.

	int localNum = 0;
	scoped_thread t{std::thread(func(localNum))};

	// c++17에서는 다음과 같이 joining_thread라는 것을 구현할 수 있음
	// 비록 이 구현은 c++17에서 C++ 표준 위원회에 의해 합의가 되지 않았지만
	// c++20에서는 std::jthread라는 이름으로 추가 되었음.

	/*
	class joining_thread
	{
		...
		(자세한 코드는 책을 참고할 것)
		...
		template<typename Callable, typename ... Args>
		explicit joining_thread(Callable&& func, Args&& ... args)
			: t(std::forward<Callable>(func), std::forward<Args>(args) ...)
		{ }
		...
	}
	*/


	// move의 지원은 thread object들을 continaer로 관리할 수 있게 해줌.
	std::vector<std::thread> threads;
	for(int i = 0 ; i < 10 ; i++)
	{
		threads.emplace_back(copyFunc, i, "hello");
	}

	for(auto& obj : threads)
	{
		obj.join();
	}

	// thread의 개수를 runtime에 결정하기
	std::cout << "hardware thread count : " << std::thread::hardware_concurrency() << std::endl;

	// 그리고 이것을 활용한 예제 ex) std::accumulate
	std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	int sum = std::accumulate(v.begin(), v.end(), 0);
	std::cout << sum << std::endl;

	// thread를 식별하기 (identigying threads)
	std::thread idThread([](){std::cout << "thread id : " << std::this_thread::get_id() << std::endl;});
	idThread.join();

	return 0;
}