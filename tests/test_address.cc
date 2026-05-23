#include "../tide/log.h"
#include "../tide/address.h"

tide::Logger::ptr g_logger = TIDE_LOG_ROOT();


void test_address()
{
    std::vector<tide::Address::ptr> addrs;

    // auto addr = tide::Address::Lookup(addrs, "www.baidu.com");
    bool v = tide::Address::Lookup(addrs, "localhost:3080");
    if(!v)
    {
        TIDE_LOG_ERROR(g_logger) << "lookup localhost:3080 failed";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i)
    {
        TIDE_LOG_INFO(g_logger) << "lookup localhost:3080: " << i << " - " << addrs[i]->toString();
    }

    auto addr = tide::Address::LookupAny("localhost:4080");
    if(addr)
    {
        TIDE_LOG_INFO(g_logger) << "lookup localhost:4080: ";
    }
    else
    {
        TIDE_LOG_ERROR(g_logger) << "lookup localhost:4080 failed";
    }
}

void test_interface()
{
    std::multimap<std::string, std::pair<tide::Address::ptr, uint32_t>> results;

    if(tide::Address::GetInterfaceAddresses(results))
    {
        for(auto &i : results)
        {
            TIDE_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - " << i.second.second;
        }
    }
}

void test_ipv4()
{
    auto addr = tide::IPAddress::Create("www.baidu.com");
    if(addr)
    {
        TIDE_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, const char*argv[])
{
    //test_address();
    // test_interface();
    test_ipv4();

    return 0;
}