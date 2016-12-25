// conc_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../../concurrent.h"


int main()
{
	using namespace conc;
	std::thread _threads[1];
	thread_queue_t queue[1];
	_threads[0] = std::thread(thread_process::proc, std::ref(queue[0]), (void*)0);
	get_next_queue_d next = [&]()-> thread_queue_t& {
		return queue[0]; };
	extract_feature_d extract = [](void*)->void* {
		return (void*)0; };
	concurrent_actions actions(next,extract,1);
	actions.add_task((void*)0);
	actions.add_done();
	actions.wait();
	_threads[0].join();
    return 0;
}

