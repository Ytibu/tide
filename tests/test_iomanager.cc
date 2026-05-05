#include "../tide/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

#include "../tide/tide.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();
//static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

std::shared_ptr<tide::Timer> s_timer;
void test_timer(tide::IOManager& iom) {
    TIDE_LOG_INFO(g_logger) << "before addTimer";
    std::weak_ptr<tide::Timer> weak_timer;
    s_timer = iom.addTimer(1000, [weak_timer]() mutable {
        static int i = 0;
        TIDE_LOG_INFO(g_logger) << "hello timer i=" << i;
        std::cout << "hello timer i=" << i << std::endl;
        auto timer = weak_timer.lock();
        if (timer) {
            if(++i == 3) {
                timer->reset(2000, true);
                //timer->cancel();
            }
        }
    }, true);
    // 让 weak_timer 捕获 s_timer
    weak_timer = s_timer;
    TIDE_LOG_INFO(g_logger) << "after addTimer";
}

int main(int argc, char **argv)
{
    tide::IOManager iom(2, true, "main"); // use_caller=true
    test_timer(iom);
    // 不需要 sleep，主线程会参与事件循环
    return 0;
}