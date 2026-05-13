#include "../tide/http/http.h"
#include "../tide/log.h"

void test_request()
{
    tide::http::HttpRequest::ptr req(new tide::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello demo");

    req->dump(std::cout) << std::endl;
}

void test_response()
{
    tide::http::HttpResponse::ptr rsp(new tide::http::HttpResponse);
    rsp->setHeader("server", "tide/1.0.0");
    rsp->setStatus(tide::http::HttpStatus::HTTP_STATUS_NOT_FOUND);
    rsp->setBody("hello demo");

    rsp->dump(std::cout) << std::endl;
}

int main()
{
    test_request();
    test_response();

    return 0;
}