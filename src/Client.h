#pragma once

#include "Proxy.h"
#include "Remote.h"
#include "IoContext.h"
#include "ConnectionStorage.h"


namespace proxy {
	class Remote;

	class Client : public std::enable_shared_from_this<Client> {
	public:
		Client () :
			strand (asio::make_strand (ioContext ())),
			socket (ioContext ())
		{}

		~Client () = default;

	public:
		tcp::socket& getSocket () { return socket; }
		ConnectionId getConnectionId () const { return socket.remote_endpoint ().address ().to_v4 ().to_uint (); };
		
		asio::ip::address & getAddress () const  { return socket.remote_endpoint ().address (); }
		uint16_t getPort () const { return socket.remote_endpoint ().port (); }


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
		tcp::socket socket;
		asio::strand<asio::io_context::executor_type> strand;

		ConnectionStorage<Remote> storage;
	};
}