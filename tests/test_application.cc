#include "../tide/application.h"
#include "../tide/log.h"

int main(int argc, char **argv)
{
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "hello application"; 
    tide::Application app;
    if (app.init(argc, argv))
    {
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "hello application";
        return app.run();
    }
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "hello application";
    return 0;
}