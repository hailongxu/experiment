#include <mutex>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <sstream>
#include <chrono>
#include <atomic>
#include <assert.h>
//#include <sysconf.h> /// get cpu count
#include "concurrent.h"


#ifndef CASE_APP_CONCURRENT_H
#define CASE_APP_CONCURRENT_H


namespace app
{

using thread_task_t = conc::group_task_sync_exit_t;
using thread_pool = conc::thread_pool;
using thread_queue_t = thread_pool::thread_queue_t;

struct case_extract
{
    conc::shared_fields shared;
    std::vector<thread_task_t> tasks;
    conc::run_d _m_run;
	size_t _m_index = 0;
	thread_pool& _m_pool;

	case_extract(thread_pool& pool,conc::run_d const& run,size_t const& task_reserve_count)
        : _m_pool(pool)
        , _m_run(run)
    {
        tasks.reserve(task_reserve_count);
    }

	void add_exit()
	{
		for (auto& i: _m_pool.queues)
		{
			i.second->add_exit_task();
		}
	}
    void add_task(void* param)
    {
        tasks.emplace_back(shared, _m_run, param);
    }
    void add_done()
    {
        //TRACE("IdlFaceProcessor::match\n");
        //auto begin = std::chrono::steady_clock::now();
        //auto begin = std::chrono::high_resolution_clock::now();
        /// with no image, return immediately
		size_t qsize = _m_pool.queues.size();
        shared.set_count(tasks.size());
        for (auto& i : tasks)
        {
            thread_queue_t* queue = _m_pool.queue(_m_index++%qsize);
			if (!queue)
			{
				ERROR2("does not exist queue:%d, belonging to first\n", (_m_index-1));
				queue = _m_pool.queue(0);
				assert(queue);
			}
            queue->add_task(&i);
        }
        //auto end = std::chrono::steady_clock::now();
        //auto end = std::chrono::high_resolution_clock::now();
        //std::chrono::duration<double, std::milli> diff = end - begin;
        //std::chrono::duration_cast<std::chrono::milliseconds> (count)
        //TRACE("======>%lf", diff.count());
    }
    void wait()
    {
		shared.wait();
        //TRACE(">>>>>>>>>>>> --------- received finished flag\n");
    }
};

} /// end of namespace

#endif
