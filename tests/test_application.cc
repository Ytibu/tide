#include "../tide/application.h"

int main(int argc, char **argv)
{
    tide::Application app;
    if (app.init(argc, argv))
    {
        return app.run();
    }

    return 0;
}