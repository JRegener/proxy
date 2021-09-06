#pragma once

#include "Proxy.h"
#include "Client.h"

namespace proxy {
	class Server : public std::enable_shared_from_this<Server> {

	public:
		Server (const tcp::endpoint& ep) :
			ioc (ioContext ()),
			endpoint (ep),
			acceptor (ioContext ()),
			socket (ioContext ())
		{ }

		~Server () = default;

	public:
		void run () {
			// TODO: error code or exception
			acceptor.open (endpoint.protocol ());
			acceptor.bind (endpoint);
			acceptor.listen (asio::socket_base::max_listen_connections);

			acceptConnection (createSession ());
		}

	private:
		Ref<Client> createSession () {
			// TODO: session storage
			return createRef<Client> ();
		}

		void acceptConnection (Ref<Client> session) {
			acceptor.async_accept (session->getSocket (),
								   std::bind (&Server::handleConnection, this, session, std::placeholders::_1));
		}

		void handleConnection (Ref<Client> session, boost::system::error_code ec) {
			if (ec) {
				// TODO:
				std::cout << ec.message () << std::endl;
				return;
			}

			session->start ();
			sessions.emplace_back (session);

			acceptConnection (createSession ());
		}

	private:
		std::vector<Ref<Client>> sessions;

		asio::io_context& ioc;
		tcp::acceptor acceptor;
		tcp::endpoint endpoint;
		tcp::socket socket;
	};
}