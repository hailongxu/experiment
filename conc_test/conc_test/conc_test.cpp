// conc_test.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "../../concurrent.h"
#include <fstream>
#include <streambuf> 
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

struct App
{
	void* feature_extract(void* p)
	{
		printf("extract [%p] 3s ...",p);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		printf("extract [%p] 3s ... DONE",p);
		
		return (void*)0;
	}
	using thread_pool = conc::thread_pool<conc::thread_group_task_t>;
	thread_pool _m_pool;

	void init()
	{
		_m_pool.init(1);
		_m_pool.start(2,1);
	}
};

int main()
{
	std::ifstream t("C:/Users/xhl/proj/idl/experiment/.gitignore", std::ifstream::in| std::ifstream::binary);
	std::string str((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());

	using namespace conc;
	App app;
	app.init();

	//case_extract::get_queue_d get = std::bind(&App::get_queue,&app,std::placeholders::_1);
	auto extract_lambda = [&](void* param, void*) {app.feature_extract(param); };
	run_d extract = std::bind(extract_lambda, (void*)0, std::placeholders::_1);

	app::case_extract actions(app._m_pool,extract,1);
	actions.add_task((void*)0);
	actions.add_task((void*)1);
	actions.add_task((void*)2);
	actions.add_task((void*)3);
	actions.add_done();
	actions.wait();
	app._m_pool.join();
    return 0;
}

