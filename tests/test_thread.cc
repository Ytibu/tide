#include "../tide/thread.h"
#include <iostream>
#include "../tide/log.h"
#include <unistd.h>

#include "../tide/config.h"

#include <vector>
#include <yaml-cpp/yaml.h>

tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

int count = 0;
tide::Mutex g_mutex;

void fun1()
{
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "thread name: " << tide::Thread::GetName()
                                   << " this.name: " << tide::Thread::GetThis()->getName()
                                   << " this.id: " << tide::Thread::GetThis()->getId();
    for (int i = 0; i < 1000; ++i)
    {
        tide::Mutex::Lock lock(g_mutex);
        ++count;
        if ((i % 1000) == 0)
        {
            usleep(1);
        }
    }
}

void fun2()
{
    while (1)
    {
        TIDE_LOG_INFO(g_logger) << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    }
}

void fun3()
{
    while (1)
    {
        TIDE_LOG_INFO(g_logger) << "￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥￥";
        for (int i = 0; i < 1000; ++i)
        {

            if ((i % 1000) == 0)
            {
                usleep(1);
            }
        }
    }
}

int main()
{

    TIDE_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node node = YAML::LoadFile("/home/dingjr/sourceCode/devlop/Tide/conf/log.yml");
    tide::Config::LoadFromYaml(node);

    std::vector<tide::Thread::ptr> thrs;
    for (int i = 0; i < 1; ++i)
    {
        tide::Thread::ptr thr(new tide::Thread(&fun2, "name_" + std::to_string(i)));
        // tide::Thread::ptr thr1(new tide::Thread(&fun3, "name_" + std::to_string(i)));
        thrs.push_back(thr);
        // thrs.push_back(thr1);
    }

    for (size_t i = 0; i < thrs.size(); ++i)
    {
        thrs[i]->join();
    }
    TIDE_LOG_INFO(g_logger) << "thread test end";
    TIDE_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}