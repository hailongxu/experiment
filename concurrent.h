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
#include "trace.h"


#ifndef APP__CONCURRENT_H
#define APP__CONCURRENT_H

namespace conc
{
	struct sys_help
	{
		static unsigned int core_count()
		{
			unsigned count = 2;
			//count = sysconf(_SC_NPROCESSORS_CONF);
			return count;
		}
        static void bind_cpu()
        {
            //thread_id = pthread_self();
        }
	};

	struct Task
	{
		virtual void run(void*) = 0;
		virtual void destroy() = 0;
	};

	struct ExitTask: Task
	{
		virtual void run(void*) {}
		virtual void destroy() {}
		static ExitTask* instance()
		{
			static ExitTask exit_task;
			return &exit_task;
		}
	};

	struct ThreadQueue
	{
		using thread_task_t = Task;
		std::mutex m;
		std::condition_variable cond;
		std::list<thread_task_t*> q;
		void add_tasks(thread_task_t* t, size_t n)
		{
			//TRACE("tasks added\n");
			std::unique_lock<std::mutex> locker(m);
			for (size_t i = 0; i < n; i++)
				q.push_back(t + i);
			cond.notify_all();
		}
		void add_task(thread_task_t* t)
		{
			//TRACE("task added\n");
			std::unique_lock<std::mutex> locker(m);
			q.push_back(t);
			cond.notify_all();
		}
		void add_exit_task()
		{
			add_task(ExitTask::instance());
		}
		void add_exit_task(size_t n)
		{
			add_tasks(ExitTask::instance(),n);
		}
	};

	using contextof_d = std::function<void*(size_t)>;
	using qidof_d = std::function<size_t(size_t)>;

	struct ThreadPool
	{
		using thread_task_t = Task;

		ThreadPool()
		{
			contextof = [](size_t) { return (void*)0; };
			qidof = [](size_t)->size_t { return 0; };
		}
		~ThreadPool()
		{
			for (auto& i : queues)
			{
				delete i.second;
			}
		}
		contextof_d contextof;
		qidof_d qidof;
		std::map<size_t, ThreadQueue*> queues;
		std::vector<std::thread> threads;

		ThreadQueue* queuebythno(size_t thno) const
		{
			size_t gi = qidof(thno);
			return queue(gi);
		}
		ThreadQueue* queue(size_t gi) const
		{
			auto i = queues.find(gi);
			if (i == queues.end())
				return (ThreadQueue*)0;
			return i->second;
		}
		void join()
		{
			for (auto& i : threads)
			{
				i.join();
			}
		}
        void add_exit_all()
        {
            for (auto& q : queues)
            {
                q.second->add_exit_task();
            }
        }
		void init(size_t thread_sum, qidof_d const& qidof)
		{
			threads.reserve(thread_sum);
			this->qidof = qidof;
		}
		void init(size_t thread_sum, size_t group_size)
		{
			qidof = [=](size_t thno)->size_t { return thno/group_size; };
			init(thread_sum, qidof);
		}
		void start()
		{
			for (size_t i = 0; i < threads.capacity(); ++i)
			{
				size_t gi = qidof(i);
				auto j = queues.find(gi);
				if (j == queues.end())
				{
					queues[gi] = new ThreadQueue;
				}
				ThreadQueue* queue = queues[gi];
				threads.emplace_back(make_thread(*queue,i,contextof(i)));
			}
		}
		static std::thread make_thread(ThreadQueue& queue, size_t poolthid,void* context = (void*)0)
		{
			return std::thread(proc, std::ref(queue), poolthid, context);
		}
		static void proc(ThreadQueue& queue,size_t poolthid, void* thread_context)
		{
			TRACE2("thread id poolthid:[%llu] sys:[%s] started\n", poolthid, dbg::thid_help::as_str().c_str());
			while (true)
			{
				/// get a task and if no, wait
				//TRACE("thread enter\n");
				thread_task_t* task = 0;
				{
					//TRACE("encounter thread locker\n");
					std::unique_lock<std::mutex> locker(queue.m);
					//TRACE("enter thread locker\n");
					while (queue.q.empty())
					{
						//TRACE("wait ============================= task being added");
						queue.cond.wait(locker);
						//TRACE("wait a task\n");
					}
					//TRACE("pop a task\n");
					task = queue.q.front();
					queue.q.pop_front();
				}

				/// do the task
				/// during the last run, it will awake the caller thread in sleeping
				if (task == ExitTask::instance())
				{
					TRACE2("thread id poolthid:[%lld] sys:[%s] is exiting \n", poolthid, dbg::thid_help::as_str().c_str());
					break;
				}
				task->run(thread_context);
				task->destroy();
			}
			TRACE2("thread id poolthid:[%lld] sys:[%s] exited\n", poolthid,dbg::thid_help::as_str().c_str());
		}
	};


	//using run_d = std::function<void(void* thread_context,void* method_context)>;


	struct SharedFields
	{
		SharedFields()
			: _m_count(0)
		{
		}
		~SharedFields()
		{
			TRACE2("~~~~~");
		}
		std::mutex _mutex_task;
		std::condition_variable _m_tasks_finished;
		std::atomic_long _m_count;
		void wait()
		{
			TRACE2("waiting...");
			std::unique_lock<std::mutex> locker(_mutex_task);
			_m_tasks_finished.wait(locker);
			TRACE2("waited. done");
		}
		void set_count(size_t const& count)
		{
			_m_count = (long)count;
		}
		int dec()
		{
			return --_m_count;
		}
		void notify_all()
		{
			_m_tasks_finished.notify_all();
		}
	};

    template <typename OF,char c=0>
	struct SyncTask: Task
	{
        /// protect all the task fields for all the tasks
        SharedFields* _m_shared_fields = 0;
        OF _m_run;

		SyncTask(OF const& run): _m_run(run)
		{
		}
		~SyncTask()
		{
			//TRACE2("~~~~~~~ thread_task_t %p\n",this);
		}

		virtual void destroy()
		{
			/// nothing
		}
		/// run in only threads
		virtual void run(void* thread_context)
		{
			//TRACE2("task to be running ....\n");
			_m_run();
			{
				//TRACE2("000000000000\n");
				//std::unique_lock<std::mutex> locker(_mutex_task);
                assert(_m_shared_fields);
				if (_m_shared_fields->dec() <= 0)
				{
					/// this will be the last thread and task's item to deal with the task
					/// there will be many thread calling this function, 
					/// but only one have the priviliage to notify the caller thread,
					/// namely just notify once by who are at run here
					//TRACE2 ("all sub taskes finished >>>>>>>>>>>>>>>>>>>>> notify ...\n");
					_m_shared_fields->notify_all();
					TRACE2(">>>>>>>>>---------notify all\n");
					//return true;
				}
			}
			//return false;
		}
	};
    /// with metthod context
    template <typename OF>
	struct SyncTask<OF,'.'>: Task
	{
        /// protect all the task fields for all the tasks
        SharedFields* _m_shared_fields = 0;
        void* _m_method_context = 0;
        OF _m_run;

		SyncTask(OF const& run,void* context)
            : _m_run(run) ,_m_method_context(context)
		{
		}
		~SyncTask()
		{
			//TRACE2("~~~~~~~ thread_task_t %p\n",this);
		}
		virtual void destroy()
		{
			/// nothing
		}
		/// run in only threads
		virtual void run(void* thread_context)
		{
			//TRACE2("task to be running ....\n");
			_m_run(_m_method_context);
			{
				//TRACE2("000000000000\n");
				//std::unique_lock<std::mutex> locker(_mutex_task);
                assert(_m_shared_fields);
				if (_m_shared_fields->dec() <= 0)
				{
					/// this will be the last thread and task's item to deal with the task
					/// there will be many thread calling this function, 
					/// but only one have the priviliage to notify the caller thread,
					/// namely just notify once by who are at run here
					//TRACE2 ("all sub taskes finished >>>>>>>>>>>>>>>>>>>>> notify ...\n");
					_m_shared_fields->notify_all();
					TRACE2(">>>>>>>>>---------notify all\n");
				}
			}
		}
	};

 //   /// with thread context
 //   template <typename OF>
	//struct SyncTask<OF,'~'>: Task
	//{
 //       /// protect all the task fields for all the tasks
 //       SharedFields* _m_shared_fields = 0;
 //       OF _m_run;

	//	SyncTask(OF const& run)
 //           : _m_run(run)
	//	{
	//	}
	//	~SyncTask()
	//	{
	//		//TRACE2("~~~~~~~ thread_task_t %p\n",this);
	//	}

	//	virtual void destroy()
	//	{
	//		/// nothing
	//	}
	//	/// run in only threads
	//	virtual void run(void* thread_context)
	//	{
	//		//TRACE2("task to be running ....\n");

	//		_m_run(thread_context);
	//		{
	//			//TRACE2("000000000000\n");
	//			//std::unique_lock<std::mutex> locker(_mutex_task);
 //               assert(_m_shared_fields);
	//			if (_m_shared_fields->dec() <= 0)
	//			{
	//				/// this will be the last thread and task's item to deal with the task
	//				/// there will be many thread calling this function, 
	//				/// but only one have the priviliage to notify the caller thread,
	//				/// namely just notify once by who are at run here
	//				//TRACE2 ("all sub taskes finished >>>>>>>>>>>>>>>>>>>>> notify ...\n");
	//				_m_shared_fields->notify_all();
	//				TRACE2(">>>>>>>>>---------notify all\n");
	//				//return true;
	//			}
	//		}
	//		//return false;
	//	}
	//};
 //   /// with thread and metthod context
 //   template <typename OF>
	//struct SyncTask<OF,'~'+'.'>: Task
	//{
 //       /// protect all the task fields for all the tasks
 //       SharedFields* _m_shared_fields = 0;
 //       void* _m_method_context = 0;
 //       OF _m_run;

	//	SyncTask(OF const& run,void* context)
 //           : _m_run(run)
 //           , _m_method_context(context)
	//	{
	//	}
	//	~SyncTask()
	//	{
	//		//TRACE2("~~~~~~~ thread_task_t %p\n",this);
	//	}

	//	virtual void destroy()
	//	{
	//		/// nothing
	//	}
	//	/// run in only threads
	//	virtual void run(void* thread_context)
	//	{
	//		//TRACE2("task to be running ....\n");

	//		_m_run(thread_context, _m_method_context);
	//		{
	//			//TRACE2("000000000000\n");
	//			//std::unique_lock<std::mutex> locker(_mutex_task);
 //               assert(_m_shared_fields);
	//			if (_m_shared_fields->dec() <= 0)
	//			{
	//				/// this will be the last thread and task's item to deal with the task
	//				/// there will be many thread calling this function, 
	//				/// but only one have the priviliage to notify the caller thread,
	//				/// namely just notify once by who are at run here
	//				//TRACE2 ("all sub taskes finished >>>>>>>>>>>>>>>>>>>>> notify ...\n");
	//				_m_shared_fields->notify_all();
	//				TRACE2(">>>>>>>>>---------notify all\n");
	//				//return true;
	//			}
	//		}
	//		//return false;
	//	}
	//};

    template <typename OF>
    static inline SyncTask<OF,0> make_sync_task(OF const& of)
    {
        return SyncTask<OF,0>(of);
    }
    template <char c,typename OF>
    static SyncTask<OF,c> make_sync_task(OF const& of,void* context)
    {
        return SyncTask<OF,c>(of, context);
    }

	struct CaseSyncTasks
	{
		conc::SharedFields shared;
		std::vector<conc::Task*> tasks;
		size_t _m_index = 0;
		
		CaseSyncTasks(size_t const& task_reserve_count=0)
		{
			tasks.reserve(task_reserve_count);
		}

		template <typename OF,char c>
		void add_task(SyncTask<OF,c>& task)
		{
            task._m_shared_fields = &shared;
			tasks.emplace_back(&task);
		}
		void add_done(conc::ThreadPool& pool)
		{
			//TRACE("IdlFaceProcessor::match\n");
			//auto begin = std::chrono::steady_clock::now();
			//auto begin = std::chrono::high_resolution_clock::now();
			/// with no image, return immediately
			size_t qsize = pool.queues.size();
			shared.set_count(tasks.size());
			for (auto& i : tasks)
			{
				ThreadQueue* queue = pool.queue(_m_index++%qsize);
				if (!queue)
				{
					ERROR2("does not exist queue:%d, belonging to first\n", (int)(_m_index-1));
					queue = pool.queue(0);
					assert(queue);
				}
				queue->add_task(i);
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

} // endof  conc


#endif
