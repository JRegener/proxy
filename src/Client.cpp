#include "Client.h"
#include "Utils.h"

namespace proxy {
	void Client::start () {
		LOG_FUNCTION_DEBUG;

		socket.start ();
		acceptRequest ();
	}

	void Client::acceptRequest () {
		LOG_FUNCTION_DEBUG;

		Ref<RequestParser> request = createRef<RequestParser> ();
		request->header_limit (std::numeric_limits<std::uint32_t>::max ());
		request->body_limit (std::numeric_limits<std::uint64_t>::max ());

		// read client request
		beast::http::async_read (socket.refresh (), buffer, *request,
								 asio::bind_executor (
									 strand,
									 std::bind (
										 &Client::handleRequest,
										 shared_from_this (),
										 request,
										 std::placeholders::_1)
								 ));
	}

	void Client::handleRequest (Ref<RequestParser> request, boost::system::error_code ec) {
		LOG_FUNCTION_DEBUG;
		std::this_thread::sleep_for (std::chrono::seconds (20));
		if (ec && !socket.isOpen ()) {
			logBoostError (ec);
			return;
		}

#if _DEBUG
		debug::Utils::printRequest (*request);
#endif

		//auto url = request->get ().target ();
		//auto method = request->get ().method ();
		//auto methodString = request->get ().method_string ();

		//for (auto& element : message) {
		//	std::cout << element.value () << std::endl;
		//}

		//request = createRef<HttpRequest> ();
		//request->version (11);
		//request->method (beast::http::verb::get);
		//request->target ("qweqweq");
		//request->set (beast::http::field::host, "google.com");
		//request->set (beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);


		auto& message = request->get ();
		beast::string_view host = message[beast::http::field::host];
		if (host.empty ()) {
			// return error;
		}

		auto [remoteHost, remotePort] = extractAddressPort (host);


		Ref<Remote> remoteSession = createRef<Remote> (*this, remoteHost, remotePort, storage);
		remoteSession->sendAsyncRequest (request);

		acceptRequest ();
	}



	void Client::sendHeaderResponseAsync (Ref<ResponseHeader> header) {
		LOG_FUNCTION_DEBUG;

		Ref<ResponseHeaderSerializer> responseSerializer =
			createRef<ResponseHeaderSerializer> (*header);

		beast::http::async_write_header (socket.refresh (),
										 *responseSerializer,
										 asio::bind_executor (
											 strand,
											 std::bind (
												 &Client::handleWriteHeader,
												 shared_from_this (),
												 header,
												 responseSerializer,
												 std::placeholders::_1,
												 std::placeholders::_2)
										 ));
	}

	void Client::sendChunkAsync (boost::string_view body) {
		LOG_FUNCTION_DEBUG;

		auto handler = [](boost::system::error_code ec, std::size_t bytes_transferred) {
			if (ec) {
				logBoostError (ec);
				return;
			}
		};

		asio::async_write (socket.refresh (), beast::http::make_chunk (asio::const_buffer (body.data (), body.size ())),
						   asio::bind_executor (
							   strand, handler
						   ));
	}

	void Client::sendChunkLastAsync () {
		LOG_FUNCTION_DEBUG;

		auto handler = [](boost::system::error_code ec, std::size_t bytes_transferred) {
			if (ec) {
				logBoostError (ec);
				return;
			}
		};

		// end of chunk
		asio::async_write (socket.refresh (), beast::http::make_chunk_last (),
						   asio::bind_executor (
							   strand, handler
						   ));
	}

	void Client::sendResponseAsync (Ref<Response> response) {
		LOG_FUNCTION_DEBUG;

		beast::http::async_write (socket.refresh (), *response,
								  asio::bind_executor (
									  strand,
									  std::bind (
										  &Client::handleWrite<Response>,
										  shared_from_this (),
										  response,
										  std::placeholders::_1,
										  std::placeholders::_2)
								  ));
	}

	std::pair<std::string, uint16_t> Client::extractAddressPort (beast::string_view host) {
		// TODO: check correct host port types
		// that port is number
		if (auto delim = host.find (':'); delim != beast::string_view::npos) {
			return std::make_pair<std::string, uint16_t> (
				std::string (host.substr (0, delim)), std::strtol (std::string (host.substr (delim + 1)).c_str (), nullptr, 10));
		}

		return std::make_pair<std::string, uint16_t> (std::string (host), 80);
	}




	template<typename T>
	void Client::handleWrite (Ref<T> response,
							  boost::system::error_code ec,
							  std::size_t bytes_transferred) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}
	}


	void Client::handleWriteHeader (Ref<ResponseHeader> responseHeader, Ref<ResponseHeaderSerializer> serializer, boost::system::error_code ec, std::size_t bytes_transferred)
	{
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}
	}
}