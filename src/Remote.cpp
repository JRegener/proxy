#include "Remote.h"

namespace proxy {

	void Remote::sendAsyncRequest (Ref<Request> request) {
		LOG_FUNCTION_DEBUG;

		if (socket.is_open ()) {
			beast::http::async_write (socket, *request,
									  asio::bind_executor (
										  strand,
										  std::bind (&Remote::readHeader,
													 shared_from_this (),
													 request,
													 std::placeholders::_1,
													 std::placeholders::_2)
									  ));
		}
		else {
			resolver.async_resolve (address, std::to_string (port),
									std::bind (&Remote::handleResolve,
											   shared_from_this (),
											   request,
											   std::placeholders::_1,
											   std::placeholders::_2));
		}
	}

	void Remote::handleResolve (Ref<Request> request,
									   boost::system::error_code ec,
									   tcp::resolver::results_type result) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}

		asio::async_connect (socket, result.begin (), result.end (),
							 asio::bind_executor (
								 strand,
								 std::bind (&Remote::handleConnect,
											shared_from_this (),
											request,
											std::placeholders::_1)));
	}

	void Remote::handleConnect (Ref<Request> request,
									   const boost::system::error_code& ec) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}

		sendAsyncRequest (request);
	}

	void Remote::readHeader (Ref<Request> request,
									boost::system::error_code ec,
									std::size_t bytes_tranferred) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}


		// в начале читаем заголовок если он chunked
		// то читаем тело запроса одним способом
		// если простой запрос просто пересылаем
		// если sse TODO

		Ref<ResponseHeaderParser> header = createRef<ResponseHeaderParser> ();
		header->header_limit (std::numeric_limits<std::uint32_t>::max ());

		beast::http::async_read_header (socket, buffer, *header,
										asio::bind_executor (
											strand,
											std::bind (&Remote::handleHeaderResponse,
													   shared_from_this (),
													   header,
													   std::placeholders::_1)
										));
	}



	void Remote::handleHeaderResponse (Ref<ResponseHeaderParser> header,
											  boost::system::error_code ec) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}

		if (header->chunked ()) {
			

		}
		else {
			std::cout << "normal response" << std::endl;
			Ref<ResponseParser> response = createRef<ResponseParser> (std::move (*header));
			response->body_limit (std::numeric_limits<std::uint64_t>::max ());

			// TODO:
			// headerhandler + bodyHandler
			// or full response handler
			// responseHandler
			// 
			// receive remote http response

		}
	}

	
}