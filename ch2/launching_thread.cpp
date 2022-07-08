#include <thread>
#include <iostream>

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
	oops();

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

	// thread function�� parameter �����ϱ�.

	return 0;
}