#include "../tide/env.h"

#include <iostream>
#include <fstream>
#include <unistd.h>

struct A
{
    A()
    {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(1024);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        for(size_t i = 0; i < content.size(); ++i)
        {
            std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
        }
    }

};

A a;

int main(int argc, char *argv[])
{
    tide::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    tide::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    tide::EnvMgr::GetInstance()->addHelp("p", "print help");

    if(!tide::EnvMgr::GetInstance()->init(argc, argv))
    {
        tide::EnvMgr::GetInstance()->printHelp();
        return -1;
    }

    std::cout << "exe: " << tide::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd: " << tide::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path: " << tide::EnvMgr::GetInstance()->getEnv("PATH", "none") << std::endl;
    std::cout << "test: " << tide::EnvMgr::GetInstance()->getEnv("test", "none") << std::endl;

    if(tide::EnvMgr::GetInstance()->has("p"))
    {
        std::cout << "help: " << std::endl;
        tide::EnvMgr::GetInstance()->printHelp();
    }

    return 0;
}