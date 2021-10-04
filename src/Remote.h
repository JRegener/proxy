#pragma once

#include "Proxy.h"
#include "Client.h"
#include "IoContext.h"
#include "ConnectionStorage.h"
#include "Socket.h"

namespace proxy {
	class Client;

	class Remote : public std::enable_shared_from_this<Remote> {
	public:
		Remote (Client & client, const std::string& address, uint16_t port, ConnectionStorage<Remote> & storage) :
			strand (asio::make_strand (ioContext ())),
			resolver (ioContext ()),
			socket (TCP_TIMEOUT_DEFAULT),
			address (address),
			port (port),
			client (client),
			storage (storage)
		{
			LOG_FUNCTION_DEBUG;
		}

		~Remote () {
			LOG_FUNCTION_DEBUG;
		}

	public:
		tcp::socket& getSocket () { return socket.getSocket (); }
		ConnectionId getConnectionId () { return getSocket ().remote_endpoint ().address ().to_v4 ().to_uint (); };

		asio::ip::address& getAddress () { return getSocket ().remote_endpoint ().address (); }
		uint16_t getPort () { return getSocket ().remote_endpoint ().port (); }
		
		void sendAsyncRequest (Ref<RequestParser> requestParser);

	private:
		void handleResolve (Ref<RequestParser> requestParser, boost::system::error_code ec, tcp::resolver::results_type result);
		void handleConnect (Ref<RequestParser> requestParser, const boost::system::error_code& ec);
		void handleWrite (Ref<Request> request, boost::system::error_code ec, std::size_t bytes_tranferred);

		void handleReadHeader (Ref<ResponseHeaderParser> header, boost::system::error_code ec);
		void handleReadBody (Ref<ResponseParser> response, boost::system::error_code ec);

		void startReadChunk (Ref<ResponseHeaderParser> header);
		void readChunk (Ref<std::string> chunk, Ref<ResponseHeaderParser> header);
		void handleReadChunk (Ref<ResponseHeaderParser> header, Ref<std::string> chunk, boost::system::error_code ec, std::size_t bytes_tranferred);

		void onChunkHeader (Ref<std::string> chunk, std::uint64_t size, boost::string_view extensions, boost::system::error_code& ec);
		size_t onChunkBody (Ref<std::string> chunk, std::uint64_t remain, boost::string_view body, boost::system::error_code& ec);

		void prepareChunk (size_t size);
		void accumulateChunk (beast::string_view body);
	private:
		Client& client;
		ConnectionStorage<Remote> & storage;

		std::string address;
		uint16_t port;

		tcp::resolver resolver;
		beast::flat_buffer buffer;
		Socket socket;

		asio::strand<asio::io_context::executor_type> strand;

		std::string ch;
	};




}