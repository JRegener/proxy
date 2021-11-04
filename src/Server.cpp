#include "Server.h"

namespace proxy {
	Server& Server::create (const tcp::endpoint& ep) {
		static Server server (ep);
		return server;
	}

	void Server::run () {
		LOG_FUNCTION_DEBUG;

		try {
			acceptor.open (endpoint.protocol ());
			acceptor.bind (endpoint);
			acceptor.listen (asio::socket_base::max_listen_connections);
		}
		catch (const boost::exception& ex) {
			logBoostError (boost::diagnostic_information (ex));

			boost::system::error_code acc_err;
			acceptor.close (acc_err);
		}

		acceptConnection (createSession ());
	}

	Ref<Client> Server::createSession () {
		return createRef<Client> ();
	}

	void Server::acceptConnection (Ref<Client> connection) {
		LOG_FUNCTION_DEBUG;

		acceptor.async_accept (connection->getSocket (),
							   std::bind (&Server::handleConnection, this, connection, std::placeholders::_1));
	}

	void Server::handleConnection (Ref<Client> connection, boost::system::error_code ec) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);

			return;
		}


		// TODO: check is ip_v6 can connect to ip_v4 server ?
		if (connectionsStorage.add (connection)) {
			connection->start ();
		}
		else {
			std::cout << "Session " << connection->getHostKey () << " already exist" << std::endl;
		}

		acceptConnection (createSession ());
	}



}

