#include "Client.h"
#include "Utils.h"
#include "HttpError.h"

namespace proxy {

void Client::destroy () {
	socket.destroy ();

	std::scoped_lock<std::mutex> lock (storageLock);
	storage.clear ();
}

void Client::setTimeoutCallback (const std::function<void ()>& callback) {
	socket.setCallback (callback);
}

void Client::start () {
	LOG_FUNCTION_DEBUG;

	socket.start ();	// start timeout timer

	acceptRequest ();
}

void Client::acceptRequest () {
	LOG_FUNCTION_DEBUG;

	Error error;
	tcp::socket& sock = socket.use (error);
	if (error == Error::SocketClosed) {
		return;
	}

	Ref<RequestParser> request = createRef<RequestParser> ();
	request->header_limit (std::numeric_limits<std::uint32_t>::max ());
	request->body_limit (std::numeric_limits<std::uint64_t>::max ());

	// read client request
	beast::http::async_read (sock, buffer, *request,
							 asio::bind_executor (
								 strand,
								 std::bind (
									 &Client::handleRequest,
									 shared_from_this (),
									 request,
									 std::placeholders::_1)
							 ));
}

static HostKey createHostKey (const std::string& host, uint16_t port) {
	return host + ':' + std::to_string (port);
}

void Client::handleRequest (Ref<RequestParser> request, boost::system::error_code ec) {
	LOG_FUNCTION_DEBUG;

	std::cout << "READ CLIENT REQUEST" << std::endl;

	if (ec) {
		logBoostError (ec);

		
		if (beast::http::error::end_of_stream == ec ||
			asio::error::operation_aborted == ec) {
			return;
		}

		sendResponseAsync (
			createRef<Response> (
				HttpError::createErrorResponse (ec, std::string_view (), request->get ().version (), request->keep_alive ())));

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

	HostKey hostKey = createHostKey (remoteHost, remotePort);

	Ref<Remote> remoteSession;

	{
		std::scoped_lock<std::mutex> lock (storageLock);

		if (storage.exist (hostKey)) {
			remoteSession.swap (storage.get (hostKey));
		}
		else {
			Ref<Remote> session = createRef<Remote> (*this, remoteHost, remotePort);

			session->setTimeoutCallback ([this, session]() {
				LOG_FUNCTION_DEBUG;

				std::scoped_lock<std::mutex> lock (storageLock);
				storage.remove (session);
										 });

			auto err = session->connect ();
			if (err) {
				std::cout << "Error create Remote connection: " << std::endl;
				logBoostError (err);
			}
			else {
				remoteSession.swap (session);
				storage.add (remoteSession);
			}
		}
	}

	if (remoteSession) remoteSession->sendAsyncRequest (request);

	acceptRequest ();
}



void Client::sendHeaderResponseAsync (Ref<ResponseHeader> header) {
	LOG_FUNCTION_DEBUG;

	Error error;
	tcp::socket& sock = socket.use (error);
	if (error == Error::SocketClosed) {
		return;
	}

	Ref<ResponseHeaderSerializer> responseSerializer =
		createRef<ResponseHeaderSerializer> (*header);

	beast::http::async_write_header (sock,
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

	Error error;
	tcp::socket& sock = socket.use (error);
	if (error == Error::SocketClosed) {
		return;
	}

	auto handler = [](boost::system::error_code ec, std::size_t bytes_transferred) {
		if (ec) {
			logBoostError (ec);
			return;
		}
	};

	asio::async_write (sock, beast::http::make_chunk (asio::const_buffer (body.data (), body.size ())),
					   asio::bind_executor (
						   strand, handler
					   ));
}

void Client::sendChunkLastAsync () {
	LOG_FUNCTION_DEBUG;

	Error error;
	tcp::socket& sock = socket.use (error);
	if (error == Error::SocketClosed) {
		return;
	}

	auto handler = [](boost::system::error_code ec, std::size_t bytes_transferred) {
		if (ec) {
			logBoostError (ec);
			return;
		}
	};

	// end of chunk
	asio::async_write (sock, beast::http::make_chunk_last (),
					   asio::bind_executor (
						   strand, handler
					   ));
}

void Client::sendResponseAsync (Ref<Response> response) {
	LOG_FUNCTION_DEBUG;

	Error error;
	tcp::socket& sock = socket.use (error);
	if (error == Error::SocketClosed) {
		return;
	}

	beast::http::async_write (sock, *response,
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