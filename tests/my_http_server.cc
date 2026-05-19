#include "../tide/http/http_server.h"
#include "../tide/iomanager.h"
#include "../tide/log.h"

tide::Logger::ptr g_logger = TIDE_LOG_ROOT();

void run()
{
	g_logger->setLevel(tide::LogLevel::INFO);
	tide::Address::ptr addr = tide::Address::LookupAnyIPAddress("0.0.0.0:8010");
	if(!addr)
	{
		TIDE_LOG_ERROR(g_logger) << "get address error";
		return;
	}
	tide::http::HttpServer::ptr http_server(new tide::http::HttpServer);
	if(!http_server->bind(addr))
	{
		TIDE_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
		sleep(2);
	}

	http_server->start();


}

int main()
{
	tide::IOManager iom(1);
	iom.schedule(run);

	return 0;
}
