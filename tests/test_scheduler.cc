#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>

#include "../tide/tide.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    TIDE_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        tide::Scheduler::GetThis()->schedule(&test_fiber, tide::GetThreadId());
    }
}

static void run_once(std::atomic<int> *count)
{
    ++(*count);
}

void test_completed_fiber_not_rerun()
{
    std::atomic<int> count{0};

    tide::Scheduler sc(1, false, "test_completed_fiber");
    tide::Fiber::ptr fiber(new tide::Fiber(std::bind(run_once, &count)));

    sc.start();
    sc.schedule(fiber);
    sc.schedule(fiber);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sc.stop();

    assert(count.load() == 1);
    TIDE_LOG_INFO(g_logger) << "scheduler regression passed, count=" << count.load();
}

int main(int argc, char** argv) {
    TIDE_LOG_INFO(g_logger) << "main";
    test_completed_fiber_not_rerun();

    tide::Scheduler sc(3, false, "test");
    sc.start();
    sleep(2);
    TIDE_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    TIDE_LOG_INFO(g_logger) << "over";
    return 0;
}