#include "../tide/daemon.h"
#include "../tide/iomanager.h"
#include "../tide/log.h"

static tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

tide::Timer::ptr timer;
int server_main(int argc, char* argv[])
{
    TIDE_LOG_INFO(g_logger) << tide::ProcessInfoMgr::GetInstance()->toString();
    tide::IOManager iom(1);
    timer = iom.addTimer(1000, [](){
        TIDE_LOG_INFO(g_logger) << "hello tide ";
        static int count = 0;
        if(++count > 10)
        {
            timer->cancel();
        }
    }, true);

    return 0;
}


int main(int argc, char* argv[])
{
    return tide::start_daemon(argc, argv, server_main, argc != 1);
}