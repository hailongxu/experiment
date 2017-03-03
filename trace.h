
#include <thread>
#include <mutex>
#include <string>
#include <sstream>
#include <map>


#pragma once


namespace dbg
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
            if (i != thids.end())
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

}


#define TRACE2(f,...) \
    do { /*break;*/ \
    std::unique_lock<std::mutex> locker(dbg::mutex_print::m()); \
    printf("[%6s] %s ",dbg::thid_help::as_str().c_str(),dbg::thids_t::instance().label()); \
    printf(f,##__VA_ARGS__);\
    fflush(stdout); \
    } while (0)
#define ERROR2 TRACE2