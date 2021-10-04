#pragma once 

#include "Proxy.h"
#include "IoContext.h"


namespace proxy {

class Socket {
public:
	Socket (int64_t seconds) : 
		socket (ioContext ()),
		timeout(ioContext (), boost::posix_time::seconds (seconds))
	{}

	Socket () :
		socket (ioContext ()),
		timeout (ioContext ())
	{}

public:
	void start ();
	void stop ();
	void setTimeout (int64_t seconds);

	bool isOpen () { return socket.is_open (); }
	
	tcp::socket& getSocket () { return socket; }
	tcp::socket& refresh () { start (); return socket; }
	ErrorConnection close ();

private:
	void handleTimeout (const boost::system::error_code & ec);

private:
	tcp::socket socket;
	asio::deadline_timer timeout;
};

}