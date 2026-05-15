#include "../tide/uri.h"
#include <iostream>

void test_uri() {
    std::string url = "http://www.example.com:8080/path/to/resource?query=param#fragment";
    tide::Uri::ptr uri = tide::Uri::Create(url);

    std::cout << "Scheme: " << uri->getScheme() << std::endl;
    std::cout << "Host: " << uri->getHost() << std::endl;
    std::cout << "Port: " << uri->getPort() << std::endl;
    std::cout << "Path: " << uri->getPath() << std::endl;
    std::cout << "Query: " << uri->getQuery() << std::endl;
    std::cout << "Fragment: " << uri->getFragment() << std::endl;
}

int main() {

    test_uri();
    return 0;
}