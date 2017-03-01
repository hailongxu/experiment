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

struct App
{
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
        pool.init(2,2);
        pool.start();
    }
};

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


void what()
{
    std::ifstream t("C:/Users/xhl/proj/idl/experiment/.gitignore", std::ifstream::in| std::ifstream::binary);
    std::string str((std::istreambuf_iterator<char>(t)),
        std::istreambuf_iterator<char>());
}


int main()
{
    cast::test();
    {
        std::function<void()> uu;
        {
            AA b;
            __int64 what = 0;
            __int64 what2 = 0;
            auto f = [=](AA const& a) {
                int aa = what;
                int bb = what2;
                printf("%d,%d,%p",a.i,b.i,&a);
            };
            uu = std::bind(f, AA());
            printf("- %zd -", sizeof(f));
            int j = 9;
        }
        uu();
        int j = 8;
    }
    printf("exit the std bind");
    getchar();
    return 9;

    using namespace conc;
    App app;
    app.init();

    conc::CaseSyncTasks actions(app.pool);
    actions.add_task(std::bind(&App::feature_extract,&app,(void*)0));
    actions.add_done();
    actions.wait();
    actions.add_exit();
    app.pool.join();
    printf("pool finished\n");
    getchar();
    return 0;
}

