#pragma once

#include "Proxy.h"
#include "Remote.h"
#include "IoContext.h"
#include "ConnectionStorage.h"
#include "Socket.h"


namespace proxy {
	class Remote;

	class Client : public std::enable_shared_from_this<Client> {
	public:
		Client () :
			strand (asio::make_strand (ioContext ())),
			socket ()
		{}

		~Client () = default;

	public:
		inline tcp::socket& getSocket () { return socket.getSocket (); }
		
		asio::ip::address & getAddress () { return getSocket ().remote_endpoint ().address (); }
		uint16_t getPort () { return getSocket ().remote_endpoint ().port (); }

		HostKey getHostKey () { return getAddress ().to_string () + ':' + std::to_string (getPort ()); }

		void start ();

		void sendHeaderResponseAsync (Ref<ResponseHeader> header);
		void sendChunkAsync (boost::string_view body);
		void sendChunkLastAsync ();

		void sendResponseAsync (Ref<Response> response);

	private:
		std::pair<std::string, uint16_t> extractAddressPort (beast::string_view host);

		void acceptRequest ();
		void handleRequest (Ref<RequestParser> request, boost::system::error_code ec);
		
		template<typename T>
		void handleWrite (Ref<T> response, boost::system::error_code ec, std::size_t bytes_transferred);
		
		void handleWriteHeader (Ref<ResponseHeader> responseHeader, Ref<ResponseHeaderSerializer> serializer, boost::system::error_code ec, std::size_t bytes_transferred);

	private:
		beast::flat_buffer buffer;
		Socket socket;
		asio::strand<asio::io_context::executor_type> strand;

		std::mutex storageLock;
		ConnectionStorage<Remote> storage;
	};
}