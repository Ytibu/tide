#include "../tide/tide.h"

tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

void run_in_fiber() {
    TIDE_LOG_INFO(g_logger) << "run_in_fiber begin";
    tide::Fiber::YieldToHold();
    TIDE_LOG_INFO(g_logger) << "run_in_fiber end";
    tide::Fiber::YieldToHold();
}

void test_fiber() {
    TIDE_LOG_INFO(g_logger) << "main begin -1";
    {
        tide::Fiber::GetThis();
        TIDE_LOG_INFO(g_logger) << "main begin";
        tide::Fiber::ptr fiber(new tide::Fiber(run_in_fiber));
        fiber->swapIn();
        TIDE_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        TIDE_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    TIDE_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char** argv) {
    tide::Thread::SetName("main");

    std::vector<tide::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(tide::Thread::ptr(
                    new tide::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    }
    return 0;
}