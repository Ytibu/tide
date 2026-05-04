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

int main(int argc, char** argv) {
    TIDE_LOG_INFO(g_logger) << "main";
    tide::Scheduler sc(3, false, "test");
    sc.start();
    sleep(2);
    TIDE_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    TIDE_LOG_INFO(g_logger) << "over";
    return 0;
}