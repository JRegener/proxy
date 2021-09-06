#pragma once

#include "Proxy.h"
#include "Client.h"

namespace proxy {
	class Client;

	class Remote : public std::enable_shared_from_this<Remote> {
	public:
		Remote (const std::string& address, uint16_t port) :
			strand (asio::make_strand (ioContext ())),
			resolver (ioContext ()),
			socket (ioContext ()),
			address (address),
			port (port)
		{
			LOG_FUNCTION_DEBUG;
		}

		~Remote () {
			LOG_FUNCTION_DEBUG;
		}

	public:
		void sendAsyncRequest (Ref<Request> request);
		

	private:
		void handleResolve (Ref<Request> request, boost::system::error_code ec, tcp::resolver::results_type result);
		void handleConnect (Ref<Request> request, const boost::system::error_code& ec);
		void readHeader (Ref<Request> request, boost::system::error_code ec, std::size_t bytes_tranferred);
		void handleHeaderResponse (Ref<ResponseHeaderParser> header, boost::system::error_code ec);

		
	private:
		
		std::string address;
		uint16_t port;

		tcp::resolver resolver;
		beast::flat_buffer buffer;
		tcp::socket socket;

		asio::strand<asio::io_context::executor_type> strand;
	};



	
}