#include "Proxy.h"
#include "Server.h"


int main (int argc, char** argv) {

	using namespace proxy;

	std::string hostAddress = "localhost";
	uint16_t hostPort = 4444;

	if (hostAddress == "localhost") {
		hostAddress = "127.0.0.1";
	}

	Ref<Server> server =
		createRef<Server> (tcp::endpoint (asio::ip::address_v4::from_string (hostAddress), hostPort));
	server->run ();

	ioContext ().run ();

	return 0;
}