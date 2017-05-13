// conc_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../../concurrent.h"
#include "../../div.h"
#include <fstream>
#include <streambuf> 
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <time.h>
#include "../../distance.h"

struct App
{
    void feat()
    {
        printf("------------\n");
    }
    void* feature_extract(void* p)
    {
        printf("extract [%p] 2s ...\n",p);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        printf("extract [%p] 2s ... DONE\n",p);
        
        return (void*)0;
    }
    conc::ThreadPool pool;

    void init()
    {
        pool.init(1,1);
        pool.start();
    }
};

void app_test()
{
    using namespace conc;
    App app;
    app.init();

    namespace ph = std::placeholders;
    conc::CaseSyncTasks actions;
    actions.add_task(conc::make_sync_task(std::bind(&App::feat, &app)));
    actions.add_task(conc::make_sync_task<'.'>(std::bind(&App::feature_extract, &app, ph::_1), 0));
    actions.add_done(app.pool);
    actions.wait();
    app.pool.add_exit_all();
    app.pool.join();
    printf("pool finished\n");
}


struct YourTask : conc::Task
{
    static YourTask* make()
    {
        return new YourTask;
    }
    virtual void run(void*)
    {
        static int i = 0;
        printf("task begin [%d] 2s ...\n",i++);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        printf("task end [%d] 2s ... DONE\n",i++);
    }
    virtual void destroy()
    {
        delete this;
    }
};
struct ThreadTest
{
    conc::ThreadPool pool;
    void init()
    {
        pool.init(1, 1);
        pool.start();
    }
    void add_task(YourTask* task)
    {
        pool.queuebythno(0)->add_task(task);
    }
    void add_exit()
    {
        pool.queuebythno(0)->add_exit_task();
    }
};
void thread_test()
{
    ThreadTest thread;
    thread.init();
    thread.add_task(YourTask::make());
    thread.add_task(YourTask::make());
    thread.add_exit();
    thread.pool.join();
    printf("***** all done!");
}

void map_test()
{
    int a = (int)ceil(4.0);
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
    TRACE2("match time cost: %llu", size_t(kk2 - kk1));
}


struct AA
{
    AA()
    {
        printf("AA(%d),%p\n",i,this);
    }
    AA(AA const& a)
    {
        i = a.i;
        printf("AA((%d)),%p\n", i,this);
    }
    int i = 9;
    ~AA()
    {
        i--;
        printf("~AA(%d),%p\n",i,this);
    }
};
struct BB
{
    AA aa[4];
};

void lambda_test()
{
    {
        std::function<void()> uu;
        {
            AA b;
            BB c;
            __int64 what = 0;
            __int64 what2 = 0;
            auto f = [=](AA  a ) {
                int aa = what;
                //int bb = what2;
                //int cc = c.aa[1].i;
                printf("%d,%d,%p", a.i, b.i, &a);
            };
            auto f2 = f;
            auto f3 = std::bind(f, AA());
            printf("- %zd - f: %zd", sizeof(uu),sizeof(f));
            uu = f3;
            int j = 9;
        }
        uu();
        int j = 8;
    }
    printf("exit the std bind");
}


void what()
{
    std::ifstream t("C:/Users/xhl/proj/idl/experiment/.gitignore", std::ifstream::in| std::ifstream::binary);
    std::string str((std::istreambuf_iterator<char>(t)),
        std::istreambuf_iterator<char>());
}

void speed()
{
    auto tt1 = std::chrono::system_clock::now();
    unsigned long long kk1 = std::chrono::duration_cast<std::chrono::milliseconds>(tt1.time_since_epoch()).count();
    TRACE2("IdlFaceProcessor::get_feature_from_process %u\n", size_t(kk1));

    char z = 0;
    char a = 10;
    char b = 11;
    char c = 10;
    char d = 11;
    char e = 10;
    char f = 11;
    for (int i = 0; i < 1000 * 1000*100; i++)
    {
        z = (int)a + (int)b + (int)c + (int)d + (int)e + (int)f;
    }
    auto tt2 = std::chrono::system_clock::now();
    unsigned long long kk2 = std::chrono::duration_cast<std::chrono::milliseconds>(tt2.time_since_epoch()).count();
    TRACE2("IdlFaceProcessor::get_feature_from_proces %u (sdk time cost: %u) \n", size_t(kk2), size_t(kk2 - kk1));
    printf("%d", c);
}


#include <string>       // std::string
std::pair<std::string,size_t> strnext(std::string const& str,size_t pos,char delim='\n')
{
    size_t p = str.find(delim, pos);
    size_t pn = p;
    if (pn != std::string::npos)
        pn++;
    return std::make_pair(str.substr(pos, p-pos),pn);
}

void split_test()
{
    std::string str = "x1111xx222x3";
    size_t pos = 100;
    while (1)
    {
        auto a = strnext(str, pos, 'x');
        std::cout << a.first << std::endl;
        pos = a.second;
        if (a.second == std::string::npos)
            break;
    }
}


int main()
{
    app_test();
    getchar();
    return 0;
    lambda_test();
    return 0;
    thread_test();
    getchar();
    return 0;
    split_test();
    return 0;
    Calc calc;
    calc.calc_many1();
    calc.calc_many2();
    return 0;
    speed();
    //cast::test();
    return 0;
}

