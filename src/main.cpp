#include "Proxy.h"
#include "Server.h"


int main (int argc, char** argv) {
#ifdef WIN32
	SetConsoleOutputCP (CP_UTF8);
#endif

	using namespace proxy;

	std::string hostAddress = "localhost";
	uint16_t hostPort = 4444;

	if (hostAddress == "localhost") {
		hostAddress = "127.0.0.1";
	}
	
	asio::executor_work_guard fakeWork = asio::make_work_guard (ioContext ());

	Server & server = Server::create(tcp::endpoint (asio::ip::address_v4::from_string (hostAddress), hostPort));
	server.run ();

	ioContext ().run ();

	return 0;
}