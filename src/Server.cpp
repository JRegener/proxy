#include "Server.h"

namespace proxy {
	Server& Server::create (const tcp::endpoint& ep) {
		static Server server (ep);
		return server;
	}

	void Server::run () {
		// TODO: error code or exception
		acceptor.open (endpoint.protocol ());
		acceptor.bind (endpoint);
		acceptor.listen (asio::socket_base::max_listen_connections);

		acceptConnection (createSession ());
	}

	Ref<Client> Server::createSession () {
		// TODO: session storage
		return createRef<Client> ();
	}

	void Server::acceptConnection (Ref<Client> session) {
		acceptor.async_accept (session->getSocket (),
							   std::bind (&Server::handleConnection, this, session, std::placeholders::_1));
	}

	void Server::handleConnection (Ref<Client> session, boost::system::error_code ec) {
		if (ec) {
			// TODO:
			std::cout << ec.message () << std::endl;
			return;
		}

		session->start ();
		sessions = session;

		acceptConnection (createSession ());
	}



}

