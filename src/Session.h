#pragma once
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
	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T, typename ... Args>
	static constexpr Ref<T> createRef (Args&& ... args) {
		return std::make_shared<T> (std::forward<Args> (args)...);
	}

#define LOG_FUNCTION_DEBUG std::cout << __FUNCTION__ <<  std::endl;

	namespace asio = boost::asio;
	namespace beast = boost::beast;
	using tcp = boost::asio::ip::tcp;

	using Request = beast::http::request<beast::http::string_body>;
	using Response = beast::http::response<beast::http::string_body>;

	using RequestParser = beast::http::request_parser<beast::http::string_body>;
	using ResponseParser = beast::http::response_parser<beast::http::string_body>;

	using RequestHeaderParser = beast::http::request_parser<beast::http::empty_body>;
	using ResponseHeaderParser = beast::http::response_parser<beast::http::empty_body>;

	static void logBoostError (const boost::system::error_code& ec) {
		std::cout << "Boost error:" << std::endl;
		std::cout << "\tCode: " << ec.value () << std::endl;
		std::cout << "\tCategory: " << ec.category ().name () << std::endl;
		std::cout << "\tMessage: " << ec.message () << std::endl;
	}

	class RemoteSession;

	class RemoteStorage {
		using RemoteSessionKey = std::string;

	public:

		~RemoteStorage () {
			LOG_FUNCTION_DEBUG;

			std::cout << "~RemoteStorage" << std::endl;
		}

		static Ref<RemoteSession> create (asio::io_context& ioc,
										  const std::string& address, uint16_t port) {
			LOG_FUNCTION_DEBUG;

			return createRef<RemoteSession> (ioc, address, port);
		}

	public:
		void add (RemoteSessionKey key, Ref<RemoteSession> session) {
			LOG_FUNCTION_DEBUG;

			auto ses = sessions.find (key);
			if (ses == sessions.end ()) {
				sessions.emplace (key, session);
			}
		}

		Ref<RemoteSession> get (RemoteSessionKey key) {
			LOG_FUNCTION_DEBUG;

			auto session = sessions.find (key);
			if (session != sessions.end ()) {
				return session->second;
			}
			return nullptr;
		}

	private:
		uint64_t calculateKey (const tcp::endpoint& ep) {

		}

	private:
		//192 168 10 2 4444
		// TODO: TMP

		std::map<RemoteSessionKey, Ref<RemoteSession>> sessions;
	};

	class RemoteSession : public std::enable_shared_from_this<RemoteSession> {
	public:
		RemoteSession (asio::io_context& ioc, const std::string& address, uint16_t port) :
			strand (asio::make_strand (ioc)),
			resolver (ioc),
			socket (ioc),
			address (address),
			port (port)
		{
			LOG_FUNCTION_DEBUG;
		}

		~RemoteSession () {
			LOG_FUNCTION_DEBUG;
		}

	public:
		void sendAsyncRequest (Ref<Request> request);

		void setResponseHandler (std::function<void (Ref<ResponseParser>,
													 boost::system::error_code,
													 std::size_t)> responseHandler) {
			LOG_FUNCTION_DEBUG;
			this->responseHandler = responseHandler;
		}

	private:
		void handleResolve (Ref<Request> request,
							boost::system::error_code ec, tcp::resolver::results_type result);

		// send request to remote server
		void handleConnect (Ref<Request> request,
							const boost::system::error_code& ec);


		void readHeader (Ref<Request> request,
						 boost::system::error_code ec,
						 std::size_t bytes_tranferred);

		void handleHeaderResponse (Ref<ResponseHeaderParser> header,
								   boost::system::error_code ec);

	private:
		std::function<void (Ref<ResponseParser>,
							boost::system::error_code,
							std::size_t)> responseHandler;

		std::string address;
		uint16_t port;

		tcp::resolver resolver;
		beast::flat_buffer buffer;
		tcp::socket socket;

		asio::strand<asio::io_context::executor_type> strand;
	};



	class ClientSession : public std::enable_shared_from_this<ClientSession> {
	public:
		ClientSession (asio::io_context& ioc, tcp::socket&& sock) :
			ioc (ioc),
			strand (asio::make_strand (ioc)),
			socket (std::move (sock))
		{}

		~ClientSession () = default;


	public:

		tcp::socket& getSocket () {
			return socket;
		}

		void start () {
			LOG_FUNCTION_DEBUG;
			acceptRequest ();
		}

	private:

		std::pair<std::string, uint16_t> extractAddressPort (beast::string_view host);

	private:
		void acceptRequest ();
		void handleRequest (Ref<RequestParser> request, boost::system::error_code ec);

#if 0
		Ref<beast::http::response_serializer<beast::http::empty_body>> serializer;
		void startSendChunk (Ref<HttpResponseHeaderParser> header) {
			resp = createRef<HttpResponseHeader> (header->get ());
			serializer = createRef<beast::http::response_serializer<beast::http::empty_body>> (*resp);

			beast::http::async_write_header (socket, *serializer,
											 asio::bind_executor (
												 strand,
												 std::bind (&ClientSession::sendChunk,
															shared_from_this (),
															header,
															resp,
															std::placeholders::_1,
															std::placeholders::_2)
											 ));
		}

		void chunkHeaderCallback (std::uint64_t size,         // Size of the chunk, or zero for the last chunk
								  beast::string_view extensions,     // The raw chunk-extensions string. Already validated.
								  boost::system::error_code& ev) {
			beast::http::chunk_extensions ce;

			std::cout << "chunkHeaderCallback" << std::endl;
			std::cout << extensions << std::endl;

			// Parse the chunk extensions so we can access them easily
			ce.parse (extensions, ev);
			if (ev)
				return;

			// See if the chunk is too big
			if (size > (std::numeric_limits<std::size_t>::max)())
			{
				ev = beast::http::error::body_limit;
				return;
			}
		}

		std::size_t chunkBodyCallback (std::uint64_t remain,   // The number of bytes left in this chunk
									   beast::string_view body,       // A buffer holding chunk body data
									   boost::system::error_code& ec) {
			std::cout << "chunkBodyCallback" << std::endl;

			// If this is the last piece of the chunk body,
			// set the error so that the call to `read` returns
			// and we can process the chunk.
			if (remain == body.size ()) {
				ec = beast::http::error::end_of_chunk;
			}

			// Append this piece to our container
			std::cout << body.data () << std::endl;

			asio::async_write (socket,
							   beast::http::make_chunk (asio::const_buffer (body.data (), body.size ())),
							   asio::bind_executor (
								   strand,
								   [](boost::system::error_code ec, size_t bytes_transferred) {
									   if (ec) {
										   // TODO: stop sending chunks?
										   return;
									   }
								   }
			));


			// The return value informs the parser of how much of the body we
			// consumed. We will indicate that we consumed everything passed in.
			return body.size ();
		}

		void sendChunk (Ref<HttpResponseHeaderParser> header,
						Ref<HttpResponseHeader> response,
						boost::system::error_code ec,
						std::size_t bytes_transferred) {
			if (header->is_done ()) {
				asio::async_write (socket,
								   beast::http::make_chunk_last (),
								   asio::bind_executor (
									   strand,
									   [](boost::system::error_code ec, size_t bytes_transferred) {
										   if (ec) {
											   // TODO: stop sending chunks?
											   return;
										   }
									   }
				));
				return;
			}

			remoteAsyncRead
				beast::http::async_read (remoteSocket, remoteBuffer, *header,
										 asio::bind_executor (
											 strand,
											 [](boost::system::error_code ec, std::size_t bytes_tranferred) {
												 if (ec) {
													 // TODO:
												 }
											 }
			));

			if (!ec) {

			}
			else if (ec != beast::http::error::end_of_chunk) {
				return;
			}
			else {
				ec = {};
	}

}
#endif
		void handleRemoteResponse (Ref<ResponseParser> response,
								   boost::system::error_code ec,
								   std::size_t bytes_tranferred);

		void handleWrite (Ref<Response> response,
						  boost::system::error_code ec,
						  std::size_t bytes_transferred);

	private:
		beast::flat_buffer buffer;
		tcp::socket socket;
		asio::strand<asio::io_context::executor_type> strand;

		asio::io_context& ioc;

		RemoteStorage remoteSessionStorage;
	};
}