#include "../tide/tide.h"

tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

void test_hook()
{
    
    tide::IOManager iom(2);

    iom.schedule([](){
        tide::set_hook_enable(true);
        sleep(2);
        TIDE_LOG_INFO(g_logger) << "test_hook sleep 2";
    });

    iom.schedule([](){
        tide::set_hook_enable(true);
        sleep(3);
        TIDE_LOG_INFO(g_logger) << "test_hook sleep 3";
    });

    TIDE_LOG_INFO(g_logger) << "test_hook finished";
}

int main()
{
    test_hook();
    return 0;
}