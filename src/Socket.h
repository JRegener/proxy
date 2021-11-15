#pragma once 

#include "Proxy.h"
#include "IoContext.h"


namespace proxy {

class Socket {
public:

	Socket () :
		socket (ioContext ()),
		timeout (ioContext ()),
		seconds (0)
	{
		LOG_FUNCTION_DEBUG;
	}

	Socket (int64_t seconds) : 
		socket (ioContext ()),
		timeout(ioContext ()),
		seconds(seconds)
	{
		LOG_FUNCTION_DEBUG;
	}

	~Socket () 
	{
		LOG_FUNCTION_DEBUG;
		
		destroy ();
	}
	


public:
	bool isOpen () { return socket.is_open (); }	
	tcp::socket& getSocket () { return socket; }

	void start ();
	void stop ();
	void expires ();
	void destroy ();
	tcp::socket& use (Error & error);
	
	void setTimeout (int64_t seconds);
	void setCallback (const std::function<void ()>& callback) { this->callback = callback; }

	boost::system::error_code close ();

private:
	void handleTimeout (const boost::system::error_code & ec);

private:
	tcp::socket socket;
	asio::deadline_timer timeout;
	std::function<void ()> callback;

	int64_t seconds;
};

}