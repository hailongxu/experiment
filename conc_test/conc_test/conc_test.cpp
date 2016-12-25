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
	std::thread _threads[1];
	conc::group_queue_t queue[1];

	void init()
	{
		_threads[0] = conc::group_help::make_thread(std::ref(queue[0]), (void*)0);
	}
	conc::group_queue_t& get_queue(size_t i)
	{
		return queue[i % (sizeof(queue) / sizeof(queue[0]))];
	}
};

int main()
{
	using namespace conc;
	App app;
	app.init();

	case_extract::get_queue_d get = std::bind(&App::get_queue,&app,std::placeholders::_1);
	void* extract_param = (void*)0;
	auto extract_lambda = [&](void* param, void*) {app.feature_extract(param); };
	run_d extract = std::bind(extract_lambda,extract_param, std::placeholders::_1);

	case_extract actions(get,extract,1);
	actions.add_task((void*)0);
	actions.add_done();
	actions.wait();
	app._threads[0].join();
    return 0;
}

