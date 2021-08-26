#include "Session.h"

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

	void RemoteSession::sendAsyncRequest (Ref<Request> request) {
		LOG_FUNCTION_DEBUG;

		if (socket.is_open ()) {
			beast::http::async_write (socket, *request,
									  asio::bind_executor (
										  strand,
										  std::bind (&RemoteSession::readHeader,
													 shared_from_this (),
													 request,
													 std::placeholders::_1,
													 std::placeholders::_2)
									  ));
		}
		else {
			resolver.async_resolve (address, std::to_string (port),
									std::bind (&RemoteSession::handleResolve,
											   shared_from_this (),
											   request,
											   std::placeholders::_1,
											   std::placeholders::_2));
		}
	}

	void RemoteSession::handleResolve (Ref<Request> request,
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
								 std::bind (&RemoteSession::handleConnect,
											shared_from_this (),
											request,
											std::placeholders::_1)));
	}

	void RemoteSession::handleConnect (Ref<Request> request,
									   const boost::system::error_code& ec) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}

		sendAsyncRequest (request);
	}

	void RemoteSession::readHeader (Ref<Request> request,
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
											std::bind (&RemoteSession::handleHeaderResponse,
													   shared_from_this (),
													   header,
													   std::placeholders::_1)
										));
	}



	void RemoteSession::handleHeaderResponse (Ref<ResponseHeaderParser> header,
											  boost::system::error_code ec) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}

		if (header->chunked ()) {
			std::cout << "chunked response" << std::endl;

			header->on_chunk_header (std::bind (&RemoteSession::onChunkHeader,
												shared_from_this (),
												std::placeholders::_1,
												std::placeholders::_2,
												std::placeholders::_3));

			header->on_chunk_body (std::bind (&RemoteSession::onChunkBody,
											  shared_from_this (),
											  std::placeholders::_1,
											  std::placeholders::_2,
											  std::placeholders::_3));

			// before reading chunk, give another handle header
			// now for send it back to client
			if (headerHandler) {
				headerHandler (header, [=]() {
					readChunk (header);
							   });
			}
			else {
				readChunk (header);
			}

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
			beast::http::async_read (socket, buffer, *response,
									 asio::bind_executor (
										 strand,
										 [=] (boost::system::error_code ec,
											  std::size_t bytes_tranferred)
										 {
											 if (responseHandler)
												 responseHandler (response, ec, bytes_tranferred);
										 }
			));
		}
	}

	void RemoteSession::onChunkHeader (std::uint64_t size,
									   beast::string_view extensions,
									   boost::system::error_code& ev) {
		LOG_FUNCTION_DEBUG;

		// chunk extensions is optional parameter
		// don't process now 

		beast::http::chunk_extensions ce;

		// Parse the chunk extensions so we can access them easily
		ce.parse (extensions, ev);
		if (ev)
			return;

		if (chunkHeaderHandler) chunkHeaderHandler (size, extensions);

		// See if the chunk is too big
		if (size > (std::numeric_limits<std::size_t>::max)())
		{
			ev = beast::http::error::body_limit;
			return;
		}
	}

	std::size_t RemoteSession::onChunkBody (std::uint64_t remain,
											beast::string_view body,
											boost::system::error_code& ec) {
		LOG_FUNCTION_DEBUG;

		// If this is the last piece of the chunk body,
		// set the error so that the call to `read` returns
		// and we can process the chunk.
		if (remain == body.size ()) {
			ec = beast::http::error::end_of_chunk;
		}

		if (chunkBodyHandler) chunkBodyHandler (remain, body);

		// The return value informs the parser of how much of the body we
		// consumed. We will indicate that we consumed everything passed in.
		return body.size ();
	}

	void RemoteSession::readChunk (Ref<ResponseHeaderParser> header) {
		LOG_FUNCTION_DEBUG;

		beast::http::async_read (socket, buffer, *header,
								 asio::bind_executor (
									 strand,
									 std::bind (&RemoteSession::handleChunkResponse,
												shared_from_this (),
												header,
												std::placeholders::_1,
												std::placeholders::_2)
								 ));
	}

	void RemoteSession::handleChunkResponse (Ref<ResponseHeaderParser> header,
											 boost::system::error_code ec, std::size_t bytes_tranferred) {
		LOG_FUNCTION_DEBUG;

		if (ec != beast::http::error::end_of_chunk) {
			std::cout << "end of chunk" << std::endl;
			return;
		}
		else {
			logBoostError (ec);
		}

		readChunk (header);
	}









	std::pair<std::string, uint16_t> ClientSession::extractAddressPort (beast::string_view host) {
		// TODO: check correct host port types
		// that port is number
		if (auto delim = host.find (':'); delim != beast::string_view::npos) {
			return std::make_pair<std::string, uint16_t> (
				std::string (host.substr (0, delim)), std::strtol (std::string (host.substr (delim + 1)).c_str (), nullptr, 10));
		}

		return std::make_pair<std::string, uint16_t> (std::string (host), 80);
	}


	void ClientSession::acceptRequest () {
		LOG_FUNCTION_DEBUG;

		Ref<RequestParser> request = createRef<RequestParser> ();
		request->header_limit (std::numeric_limits<std::uint32_t>::max ());
		request->body_limit (std::numeric_limits<std::uint64_t>::max ());

		// read client request
		beast::http::async_read (socket, buffer, *request,
								 asio::bind_executor (
									 strand,
									 std::bind (
										 &ClientSession::handleRequest,
										 shared_from_this (),
										 request,
										 std::placeholders::_1)
								 ));
	}

	void ClientSession::handleRequest (Ref<RequestParser> request, boost::system::error_code ec) {
		LOG_FUNCTION_DEBUG;

		if (ec && !socket.is_open ()) {
			logBoostError (ec);
			return;
		}

		// тут парсим запрос и перенаправляем на целевой сервер
		// нужно посмотреть в каком виде приходит запрос отправленный через прокси
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

		// один пользователь может иметь множество подключений 
		// если в заголовке возвращается keep-alive то данное подключение разрывает либо
		// удаленный сервер либо прокси по истечении времени
		// 
		// поиск удаленного подключения временно производится по хосту и порту

		auto& message = request.get ()->get ();
		beast::string_view host = message[beast::http::field::host];
		if (host.empty ()) {
			// return error;
		}

		auto [remoteHost, remotePort] = extractAddressPort (host);


		Ref<Request> remoteRequest = createRef<Request> (request->get ());


		std::string sessionKey = remoteHost + ":" + std::to_string (remotePort);
		Ref<RemoteSession> session;
		if (session = remoteSessionStorage.get (sessionKey); !session) {
			session = RemoteStorage::create (ioc, remoteHost, remotePort);
			// создаем удаленное подключение 
			// мы должны добавить сессию в хранилище после получения ip
			// TODO: TMP
			remoteSessionStorage.add (sessionKey, session);
		}
		session->setResponseHandler (std::bind (&ClientSession::forwardRemoteResponse,
												shared_from_this (),
												std::placeholders::_1,
												std::placeholders::_2,
												std::placeholders::_3));
		session->setHeaderHandler (std::bind (&ClientSession::headerHandler,
											  shared_from_this (),
											  std::placeholders::_1,
											  std::placeholders::_2));
		session->setChunkHeaderHandler (std::bind (&ClientSession::chunkHeaderHandler,
												   shared_from_this (),
												   std::placeholders::_1,
												   std::placeholders::_2));
		session->setChunkBodyHandler (std::bind (&ClientSession::chunkBodyHandler,
												 shared_from_this (),
												 std::placeholders::_1,
												 std::placeholders::_2));

		session->sendAsyncRequest (remoteRequest);
	}

	void ClientSession::forwardRemoteResponse (Ref<ResponseParser> response,
											   boost::system::error_code ec,
											   std::size_t bytes_tranferred) {
		LOG_FUNCTION_DEBUG;

		// получаем ответ от сервера и перенаправляем обратно клиенту
		if (ec) {
			logBoostError (ec);
			return;
		}

		Ref<Response> clientResponse = createRef<Response> (response->get ());

		beast::http::async_write (socket, *clientResponse,
								  asio::bind_executor (
									  strand,
									  std::bind (&ClientSession::handleWrite,
												 shared_from_this (),
												 clientResponse,
												 std::placeholders::_1,
												 std::placeholders::_2)
								  ));
	}

	void ClientSession::handleWrite (Ref<Response> clientResponse,
									 boost::system::error_code ec,
									 std::size_t bytes_transferred) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}
	}

	void ClientSession::headerHandler (Ref<ResponseHeaderParser> header,
									   std::function<void ()> callback) {
		LOG_FUNCTION_DEBUG;

		forwardHeader (createRef<ResponseHeader> (header->get ()),
					   [&](boost::system::error_code, size_t) {
						   callback ();
					   });
	}

	void ClientSession::chunkHeaderHandler (std::uint64_t size, beast::string_view extensions) {
		LOG_FUNCTION_DEBUG;


	}

	void ClientSession::chunkBodyHandler (std::uint64_t remain, beast::string_view body) {
		LOG_FUNCTION_DEBUG;

		if (remain == body.size ()) {
			// end of chunk
			forwardChunkBody (beast::http::make_chunk_last ());
		}

		forwardChunkBody (beast::http::make_chunk (asio::const_buffer (body.data (), body.size ())));
	}

	void ClientSession::forwardHeader (Ref<ResponseHeader> header,
									   std::function<void (boost::system::error_code, size_t)> handler) {
		LOG_FUNCTION_DEBUG;

		beast::http::response_serializer <beast::http::empty_body> serializer (*header);

		beast::http::async_write_header (socket, serializer,
										 asio::bind_executor (
											 strand,
											 [=](boost::system::error_code ec, size_t bytes_transferred) {
												 handler (ec, bytes_transferred);
											 }
										 ));
	}

	template<typename T>
	void ClientSession::forwardChunkBody (const T& chunk) {
		LOG_FUNCTION_DEBUG;

		asio::async_write (socket,
						   chunk,
						   asio::bind_executor (
							   strand,
							   [](boost::system::error_code ec, size_t bytes_transferred) {
								   if (ec) {
									   logBoostError (ec);
									   // TODO: stop sending chunks?
									   return;
								   }
							   }));
	}
}