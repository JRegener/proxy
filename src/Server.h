#pragma once

#include "Proxy.h"
#include "IoContext.h"
#include "Client.h"

namespace proxy {
	class Server : public std::enable_shared_from_this<Server> {
	public:
		static Server& create (const tcp::endpoint& ep);

	public:
		Server (const tcp::endpoint& ep) :
			endpoint (ep),
			acceptor (ioContext ()),
			socket (ioContext ())
		{ }

		~Server () = default;

	public:
		void run ();

	private:
		Ref<Client> createSession ();

		void acceptConnection (Ref<Client> session);

		void handleConnection (Ref<Client> session, boost::system::error_code ec);

	private:
		Ref<Client> sessions;

		tcp::acceptor acceptor;
		tcp::endpoint endpoint;
		tcp::socket socket;
	};
}