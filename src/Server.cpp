#include "Server.h"

namespace proxy {
	Server& Server::create (const tcp::endpoint& ep) {
		static Server server (ep);
		return server;
	}

	void Server::run () {
		LOG_FUNCTION_DEBUG;

		// TODO: error code or exception
		acceptor.open (endpoint.protocol ());
		acceptor.bind (endpoint);
		acceptor.listen (asio::socket_base::max_listen_connections);

		acceptConnection (createSession ());
	}

	Ref<Client> Server::createSession () {
		return createRef<Client> ();
	}

	void Server::acceptConnection (Ref<Client> session) {
		LOG_FUNCTION_DEBUG;

		acceptor.async_accept (session->getSocket (),
							   std::bind (&Server::handleConnection, this, session, std::placeholders::_1));
	}

	void Server::handleConnection (Ref<Client> session, boost::system::error_code ec) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			// TODO:
			logBoostError (ec);
			return;
		}

		session->start ();
		sessions = session;

		acceptConnection (createSession ());
	}



}

