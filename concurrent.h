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

#ifndef APP_SEARCH_VIS_VISARCH_FEATURES_CONCURRENT_H
#define APP_SEARCH_VIS_VISARCH_FEATURES_CONCURRENT_H

namespace conc
{

struct thid_help
{
    static std::string as_str(std::thread::id const& id)
    {
        std::stringstream ss;
        ss << id;
        return ss.str();
    }
    static std::string as_str()
    {
        return as_str(std::this_thread::get_id());
    }
};

struct thids_t
{
    std::map<std::thread::id, std::string> thids;
    void add(std::thread::id const& id, char const* name)
    {
        printf("==== [%s] - [%s] \n", thid_help::as_str(id).c_str(), name);
        thids[id] = name;
    }
    char const* label() const
    {
        auto i = thids.find(std::this_thread::get_id());
        if (i!= thids.end())
            return i->second.c_str();
        return "********";
    }

    static thids_t& instance()
    {
        static thids_t _s_thids;
        return _s_thids;
    }
};


struct mutex_print
{
    static std::mutex& m()
    {
        static std::mutex _g_mutex_print;
        return _g_mutex_print;
    }
};



#define TRACE2(f,...) \
    do { /*break;*/ \
    std::unique_lock<std::mutex> locker(mutex_print::m()); \
    printf("[%6s] %s ",thid_help::as_str().c_str(),thids_t::instance().label()); \
    printf(f,##__VA_ARGS__);\
    fflush(stdout); \
    } while (0)

/// the original codes simulated/spicy
/// {{{

//using ImageItem = vis::ImageItem;
//using IdlFaceRequest = vis::IdlFaceRequest;
//using IdlFaceResponse = vis::IdlFaceResponse ;
//using FeatureRequest = vis::FeatureRequest; 
/*
struct ImageItem
{
    std::string img;
};

struct IdlFaceRequest
{
    ~IdlFaceRequest()
    {
        TRACE ("~~~~IdlFaceRequest%p\n",this);
    }
    std::vector<ImageItem> _m_images;
    size_t images_size() const
    {
        return _m_images.size();
    }
    ImageItem images(int i) const
    {
        return _m_images[i];
    }
};
struct IdlFaceResponse
{
};

struct FeatureRequest 
{
};
*/
//std::function<int(const uint64_t &logid,const FeatrueRequest& req_fea, std::string& res_fea)> get_feature_from_process;
/*
void get_feature_from_process(unsigned long long, FeatureRequest,std::string& res_fea)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
void prepare_feature_request(FeatureRequest, ImageItem&)
{
}
int multil_match(int, std::vector<std::string>& vec_fea, IdlFaceResponse& response)
{
    return 0;
}
*/

//// }}}

struct shared_fields
{
    shared_fields()
        : _m_count(0)
    {
    }
	~shared_fields()
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
        _m_count = count;
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
using extract_feature_d = std::function<void*(void*)>;
struct thread_task_t
{
    thread_task_t(
        shared_fields* shared,
        extract_feature_d const& extract_feature,
        void* param
        )
        : _m_shared_fields(shared)
        , _m_extract_feature (extract_feature)
        , _m_param(param)
    {
    }
    ~thread_task_t()
    {
        //TRACE2("~~~~~~~ thread_task_t %p\n",this);
    }

    /// protect all the task fields for all the tasks
    shared_fields* _m_shared_fields;
    extract_feature_d _m_extract_feature;
    void* _m_param;

    /// run in only threads
    bool run(void* thread_context)
    {
        //TRACE2("task to be running ....\n");

        _m_extract_feature(_m_param);
        {
            //TRACE2("000000000000\n");
            //std::unique_lock<std::mutex> locker(_mutex_task);
            if (_m_shared_fields->dec() <= 0)
            {
                /// this will be the last thread and task's item to deal with the task
                /// there will be many thread calling this function, 
                /// but only one have the priviliage to notify the caller thread,
                /// namely just notify once by who are at run here
                //TRACE2 ("all sub taskes finished >>>>>>>>>>>>>>>>>>>>> notify ...\n");
				_m_shared_fields->notify_all();
                TRACE2 (">>>>>>>>>---------notify all\n");
                return true;
            }
        }
        return false;
    }
};



struct thread_queue_t
{
    std::mutex m;
    std::condition_variable cond;
    std::list<thread_task_t*> q;
    void add_tasks(thread_task_t* t, size_t n)
    {
        //TRACE("tasks added\n");
        std::unique_lock<std::mutex> locker(m);
        for (size_t i = 0; i<n; i++)
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
};

struct thread_process
{
    static void proc(thread_queue_t& queue,void* thread_context)
    {
        while (1)
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
            bool b = task->run(thread_context);
            (void)(b);
        }
    }
};

using get_next_queue_d = std::function<thread_queue_t&()>;
struct concurrent_actions
{
    get_next_queue_d next_queue;

    shared_fields shared;
    std::vector<thread_task_t> tasks;
    extract_feature_d extract_feature;

	concurrent_actions(get_next_queue_d const& next,extract_feature_d const& extract,size_t const& task_reserve_count)
        : next_queue(next)
        , extract_feature(extract) 
    {
        tasks.reserve(task_reserve_count);
    }

    void add_task(void* param)
    {
        tasks.emplace_back(&shared, extract_feature, param);
    }
    void add_done()
    {
        //TRACE("IdlFaceProcessor::match\n");
        //auto begin = std::chrono::steady_clock::now();
        //auto begin = std::chrono::high_resolution_clock::now();
        /// with no image, return immediately
        shared.set_count(tasks.size());
        for (auto i : tasks)
        {
            thread_queue_t& queue = next_queue();
            queue.add_task(&i);
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
