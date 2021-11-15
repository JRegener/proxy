#include "Server.h"

namespace proxy {
	Server& Server::getInstance (const tcp::endpoint& ep) {
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

		if (!ec) {
			{
				std::scoped_lock<std::mutex> lock (storageLock);

				connection->setTimeoutCallback ([this, connection]() {
					LOG_FUNCTION_DEBUG;

					std::scoped_lock<std::mutex> lock (storageLock);
					storage.remove (connection);
												});

				storage.add (connection);
				connection->start ();
			}
		}

		if (ec) logBoostError (ec);

		acceptConnection (createSession ());
	}



}

