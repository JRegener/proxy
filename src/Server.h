#pragma once

#include "Proxy.h"
#include "IoContext.h"
#include "Client.h"
#include "ConnectionStorage.h"

namespace proxy {
	class Server : public std::enable_shared_from_this<Server> {
	public:
		static Server& getInstance (const tcp::endpoint& ep);

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

		void acceptConnection (Ref<Client> connection);

		void handleConnection (Ref<Client> connection, boost::system::error_code ec);

	private:
		std::mutex storageLock;
		ConnectionStorage<Client> storage;


		tcp::acceptor acceptor;
		tcp::endpoint endpoint;
		tcp::socket socket;
	};
}