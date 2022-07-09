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
			// do_something �Լ� ���ο���
			// reference i�� ���� ��
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

	// function return �Ǹ�
	// localVriable �����ǰ�, thread�� �����־ dangling �߻�.
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
	* ���� : most vexing parse!!
	* �Ʒ� �ڵ�� �������� �ʽ��ϴ�.
	*/

	std::thread my_thread2(background_task());
	// my_thread2.join(); // Error !!

	/*
	* most vexing parse ȸ�� ��� 2����
	*/
	std::thread my_thread3((background_task()));
	my_thread3.join();

	std::thread my_thread4((background_task()));
	my_thread4.join();

	/*
	* Lambda ����ϱ�
	*/

	std::thread lambda_thread([]() { std::cout << "thread joined" << std::endl; });
	lambda_thread.join();

	/*
	* �ϴ�, thread�� �����ϸ� thread�� finish �� ������ ��ٸ��� (join)
	* �Ǵ� ���� ���� thread�� leave �ؾ������� �����ؾ� �Ѵ�. (detach)
	* ���� 2������ �����ϱ� ���� thread�� destory �ȴٸ�, program�� std::terminate()�� ȣ���Ѵ�.
	*/

	std::thread lambda_thread2([]() { std::cout << "thread detached" << std::endl; });
	lambda_thread2.detach(); // detach �ϸ�, thread ���� ���� �� ��.

	/*
	* ���� ������ thread�� wait���� �ʴ´ٸ�
	* thread�� ���� ���� �Ǵ� data�� thread�� finish �ɶ� ���� ��ȿ���� �����ؾ߸� �Ѵ�.
	* �׷��� ������ thread�� object�� ���� �� ��, object�� destory �� ��� undefined behavior�� �߻��Ѵ�.
	* (�̰��� ��� ���ο� �̽��� �ƴϴ�, single thread ȯ�濡���� �����̴�.)
	*/

	/*
	* ������, thread���� �����, �����ϴ� ���� �ֱ�(lifetime) �̽��鿡 ����
	* �߰����� ��ȸ�� �����Ѵ�.
	*/

	/*
	 * thread�� ������ ���� ������ �����ϴ� ����, function�� return�ϴ� ��Ȳ
	 * (�Ʒ� oops�� dangling �̽� �߻�.)
	 */

	// �� Scenario�� ó���ϴ� ����� ������ ����.
	// 1) sharing data�� ������� �ʰ� copy�� data�� ����ϴ� �� (Bad Idea)
	// 2) join ���
	//    - �ܼ� join ���
	//    - RAII ������ ����ϴ� thread_guard Wrapper ���

	// undefined behavior!!!!!!
	// oops();

	// detach�� ȣ���ϴ� ���� thread�� background�� ������
	// �������� ����� ���ٴ� �ǹ��̴�. �� �̻� thread�� �Ϸ� �� ������ ��ٸ��� ���� �Ұ����ϴٴ� ���̴�. (join �Ұ���)
	// detach thread�� daemon thread(in UNIX)��� �Ҹ���. user�� ����� interface ���� background���� �����Ѵ�.
	// file-system�� ����͸��ϰų�, clearing ������ �����ϰų�, ĳ�� ���� Ȥ�� data structure ����ȭ ���� �����Ѵ�.

	// detach thread�� ����ϴ� ���� thread�� �Ϸ�Ǿ��� ���� �ĺ��ϱ� ���� �ٸ� ��Ŀ���� ���ǰų�
	// fire-and-forget task ��Ŀ� ����ϴ� ���� Ÿ���� �� �����ϴ�.

	/* detach Ȱ�� ��.
	void edit_document(std::string const& filename)
	{
		open_document_and_display_gui(filename);

		while (!done.editing())
		{
			user_command cmd = get_user_input();

			if (cmd.type == open_new_document)
			{
				// ���� ���带 �߰��� open�ϸ�
				// ������ thread�� detach�� ���ָ� �ȴ�!!!.
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
	* thread function�� parameter �����ϱ�.
	*/

	// �Ʒ� ������ �����δ� �����Ϸ��� ���� Copy�� �߻���.
	std::thread copyThread(copyFunc, 1, "hello");
	copyThread.join();

	// �Ʒ��� ���� ����ؾ� ��.
	// void update_data_for_widget(widget_id w, widget_data& data);
	// std::thread t(update_data_for_widget, w, data);

	std::string refStr = "ref";
	//std::thread refThread(refFunc, 2, refStr); //  ��� ���� ���� ��.
	std::thread refThread(refFunc, 3, std::ref(refStr)); // �̷��� ��� ��
	refThread.join();

	//std::bind�� ��� ����� ���Ƽ�, �Ʒ�ó���� ��� ����.
	// Ŭ���� ���ο����� �Ʒ�ó���� ��� ����.
	// std::thread bindThread(&X::do_something, this);
	// �ڵ忡�� my_x.do_something()�� ���� �� ����.
	X my_x;
	std::thread bindThread(&X::do_something, &my_x);
	bindThread.join();

	// �̹��� ����� �� �� ���� move�� ������ object�� ���� Case ��.
	std::unique_ptr<big_object> p = std::make_unique<big_object>("big_object");
	std::cout << p << std::endl;
	std::thread moveThread(process_big_object, std::move(p));
	moveThread.join();

	// thread�� ������ �̵�
	std::thread transferring1(some_function);
	std::thread transferring2 = std::move(transferring1);
	transferring1 = std::thread(some_other_function);

	std::thread transferring3;
	transferring3 = std::move(transferring2);
	
	// transferring1�� �̹� �������� ������ ����.
	// std::terminate() �߻�
	// transferring1 = std::move(transferring3);

	transferring1.join();
	transferring3.join();

	// move�� ������
	// thread_guard�� std::move�� ������ �� �ִٴ� ���̴�.

	int localNum = 0;
	scoped_thread t{std::thread(func(localNum))};

	// c++17������ ������ ���� joining_thread��� ���� ������ �� ����
	// ��� �� ������ c++17���� C++ ǥ�� ����ȸ�� ���� ���ǰ� ���� �ʾ�����
	// c++20������ std::jthread��� �̸����� �߰� �Ǿ���.

	/*
	class joining_thread
	{
		...
		(�ڼ��� �ڵ�� å�� ������ ��)
		...
		template<typename Callable, typename ... Args>
		explicit joining_thread(Callable&& func, Args&& ... args)
			: t(std::forward<Callable>(func), std::forward<Args>(args) ...)
		{ }
		...
	}
	*/


	// move�� ������ thread object���� continaer�� ������ �� �ְ� ����.
	std::vector<std::thread> threads;
	for(int i = 0 ; i < 10 ; i++)
	{
		threads.emplace_back(copyFunc, i, "hello");
	}

	for(auto& obj : threads)
	{
		obj.join();
	}

	// thread�� ������ runtime�� �����ϱ�
	std::cout << "hardware thread count : " << std::thread::hardware_concurrency() << std::endl;

	// �׸��� �̰��� Ȱ���� ���� ex) std::accumulate
	std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	int sum = std::accumulate(v.begin(), v.end(), 0);
	std::cout << sum << std::endl;

	// thread�� �ĺ��ϱ� (identigying threads)
	std::thread idThread([](){std::cout << "thread id : " << std::this_thread::get_id() << std::endl;});
	idThread.join();

	return 0;
}