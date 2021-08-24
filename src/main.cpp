#include "Session.h"
#include <iostream>
#include <functional>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#pragma warning (disable: 4996 4083)
/*
* Аутентификация
*
* принимаем подключение пользователя
* читаем пользовательский запрос
* парсим его
* перенаправляем на целевой сервер
* получаем ответ
* возвращаем пользователю
*/

namespace proxy {


	class Server {
	public:
		static Server& create (asio::io_context& ioc,
							   const std::string& address, const uint16_t port) {
			// TODO
			static Server server (ioc, tcp::endpoint (asio::ip::address_v4::from_string (address), port));
			return server;
		}

	public:
		Server (asio::io_context& ioc, const tcp::endpoint& ep) :
			ioc (ioc),
			endpoint (ep),
			acceptor (ioc),
			socket (ioc)
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
		Ref<ClientSession> createSession () {
			// TODO: session storage
			return createRef<ClientSession> (ioc, std::move (socket));
		}

		void acceptConnection (Ref<ClientSession> session) {
			acceptor.async_accept (session->getSocket (),
								   std::bind (&Server::handleConnection, this, session, std::placeholders::_1));
		}

		void handleConnection (Ref<ClientSession> session, boost::system::error_code ec) {
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
		std::vector<Ref<ClientSession>> sessions;

		asio::io_context& ioc;
		tcp::acceptor acceptor;
		tcp::endpoint endpoint;
		tcp::socket socket;
	};
}


int main (int argc, char** argv) {



	std::string hostAddress = "localhost";
	uint16_t hostPort = 4444;

	if (hostAddress == "localhost") {
		hostAddress = "127.0.0.1";
	}

	boost::asio::io_context ioc;

	proxy::Server& server =
		proxy::Server::create (ioc, hostAddress, hostPort);
	server.run ();
	ioc.run ();

	return 0;
}