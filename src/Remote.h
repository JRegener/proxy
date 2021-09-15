#pragma once

#include "Proxy.h"
#include "Client.h"
#include "IoContext.h"

namespace proxy {
	class Client;

	class Remote : public std::enable_shared_from_this<Remote> {
	public:
		Remote (Ref<Client> client, const std::string& address, uint16_t port) :
			strand (asio::make_strand (ioContext ())),
			resolver (ioContext ()),
			socket (ioContext ()),
			address (address),
			port (port),
			client (client)
		{
			LOG_FUNCTION_DEBUG;
		}

		~Remote () {
			LOG_FUNCTION_DEBUG;
		}

	public:
		void sendAsyncRequest (Ref<Request> request);

		void prepareChunk (size_t size);
		void accumulateChunk (beast::string_view body);
	private:
		void handleResolve (Ref<Request> request, boost::system::error_code ec, tcp::resolver::results_type result);
		void handleConnect (Ref<Request> request, const boost::system::error_code& ec);
		void handleWrite (Ref<Request> request, boost::system::error_code ec, std::size_t bytes_tranferred);

		void handleReadHeader (Ref<ResponseHeaderParser> header, boost::system::error_code ec);
		void handleReadBody (Ref<ResponseParser> response, boost::system::error_code ec);

		void readChunk (Ref<ResponseHeaderParser> header);
		void handleReadChunk (Ref<ResponseHeaderParser> header, Ref<std::string> chunk, boost::system::error_code ec, std::size_t bytes_tranferred);

		void onChunkHeader (std::uint64_t size,
							boost::string_view extensions,
							boost::system::error_code& ec);

		size_t onChunkBody (std::uint64_t remain,
							boost::string_view body,
							boost::system::error_code& ec);

	private:
		Ref<Client> client;

		std::string address;
		uint16_t port;

		tcp::resolver resolver;
		beast::flat_buffer buffer;
		tcp::socket socket;

		asio::strand<asio::io_context::executor_type> strand;

		std::string ch;
	};




}