#include "../tide/log.h"
#include "../tide/address.h"

tide::Logger::ptr g_logger = TIDE_LOG_ROOT();


void test_address()
{
    std::vector<tide::Address::ptr> addrs;

    auto addr = tide::Address::Lookup(addrs, "www.baidu.com");
    if(!addr)
    {
        TIDE_LOG_ERROR(g_logger) << "lookup www.baidu.com failed";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i)
    {
        TIDE_LOG_INFO(g_logger) << "lookup www.baidu.com: " << i << " - " << addrs[i]->toString();
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