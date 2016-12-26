// conc_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../../concurrent.h"


struct App
{
	void* feature_extract(void*)
	{
		printf("%s\n", "extract");
		return (void*)0;
	}
	using thread_pool = conc::thread_pool<conc::thread_group_task_t>;
	thread_pool _m_pool;
	//std::thread _threads[1];
	//conc::group_queue_t queue[1];

	void init()
	{
		_m_pool.start(1,1);
	}
};

int main()
{
	using namespace conc;
	App app;
	app.init();

	//case_extract::get_queue_d get = std::bind(&App::get_queue,&app,std::placeholders::_1);
	auto extract_lambda = [&](void* param, void*) {app.feature_extract(param); };
	run_d extract = std::bind(extract_lambda, (void*)0, std::placeholders::_1);

	case_extract actions(app._m_pool,extract,1);
	actions.add_task((void*)0);
	actions.add_done();
	actions.wait();
	app._m_pool._m_threads.front().join();
    return 0;
}

