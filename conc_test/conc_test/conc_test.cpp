// conc_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../../concurrent.h"
#include <fstream>
#include <streambuf> 
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <time.h>

struct App
{
	void* feature_extract(void* p)
	{
		printf("extract [%p] 2s ...\n",p);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		printf("extract [%p] 2s ... DONE\n",p);
		
		return (void*)0;
	}
	conc::ThreadPool _m_pool;

	void init()
	{
		_m_pool.init(2,2);
		_m_pool.start();
	}
};

void map_test()
{
	int a = ceil(4.0);
	using T = std::map<std::string, std::string>;
	T m;
	std::string key, value;
	for (int i = 0; i < 100*1000; i++)
	{
		char buf[64];
		sprintf_s(buf, "key:%d", i);
		key = buf;
		sprintf_s(buf, "value:%d", i);
		value = buf;
		m[key] = value;
	}

	auto tt1 = std::chrono::system_clock::now();

	std::vector<T::value_type*> v;
	v.reserve(m.size());
	for (auto& i:m)
	{
		v.push_back(&i);
	}

	auto tt2 = std::chrono::system_clock::now();
	unsigned long long kk1 = std::chrono::duration_cast<std::chrono::milliseconds>(tt1.time_since_epoch()).count();
	unsigned long long kk2 = std::chrono::duration_cast<std::chrono::milliseconds>(tt2.time_since_epoch()).count();
	TRACE2("match time cost: %u", size_t(kk2 - kk1));
}


void what()
{
	std::ifstream t("C:/Users/xhl/proj/idl/experiment/.gitignore", std::ifstream::in| std::ifstream::binary);
	std::string str((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
}


int main()
{
	using namespace conc;
	App app;
	app.init();

	//case_extract::get_queue_d get = std::bind(&App::get_queue,&app,std::placeholders::_1);
	auto extract_lambda = [&](void* param, void*) {app.feature_extract(param); };
	run_d extract = std::bind(extract_lambda, (void*)0, std::placeholders::_1);

	conc::CaseSyncTasks actions(app._m_pool,extract,1);
	actions.add_task((void*)0);
	actions.add_task((void*)1);
	actions.add_task((void*)2);
	actions.add_task((void*)3);
	actions.add_done();
	actions.add_exit();
	actions.wait();
	app._m_pool.join();
	printf("pool finished\n");
	getchar();
    return 0;
}

