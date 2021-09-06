#pragma once

#include "Proxy.h"
#include "Remote.h"


namespace proxy {
	class Remote;

	class Client : public std::enable_shared_from_this<Client> {
	public:
		Client () :
			ioc (ioContext ()),
			strand (asio::make_strand (ioContext ())),
			socket (ioContext ())
		{}

		~Client () = default;

	public:
		tcp::socket& getSocket () { return socket; }

		void start () {
			LOG_FUNCTION_DEBUG;
			acceptRequest ();
		}

	private:
		std::pair<std::string, uint16_t> extractAddressPort (beast::string_view host);

		void acceptRequest ();
		void handleRequest (Ref<RequestParser> request, boost::system::error_code ec);
		void handleWrite (Ref<Response> response, boost::system::error_code ec, std::size_t bytes_transferred);

	private:
		beast::flat_buffer buffer;
		tcp::socket socket;
		asio::strand<asio::io_context::executor_type> strand;

		asio::io_context& ioc;

	};
}