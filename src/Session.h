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

	using RequestHeader = beast::http::request<beast::http::empty_body>;
	using ResponseHeader = beast::http::response<beast::http::empty_body>;

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
													 std::size_t)> handler) {
			LOG_FUNCTION_DEBUG;
			this->responseHandler = handler;
		}

		void setHeaderHandler (std::function<void (Ref<ResponseHeaderParser>, std::function<void ()>)> handler) {
			LOG_FUNCTION_DEBUG;
			this->headerHandler = handler;
		}

		void setChunkHeaderHandler (std::function<void (std::uint64_t, beast::string_view)> handler) {
			LOG_FUNCTION_DEBUG;
			this->chunkHeaderHandler = handler;
		}

		void setChunkBodyHandler (std::function<void (std::uint64_t, beast::string_view)> handler) {
			LOG_FUNCTION_DEBUG;
			this->chunkBodyHandler = handler;
		}

	private:
		void handleResolve (Ref<Request> request, boost::system::error_code ec, tcp::resolver::results_type result);
		void handleConnect (Ref<Request> request, const boost::system::error_code& ec);
		void readHeader (Ref<Request> request, boost::system::error_code ec, std::size_t bytes_tranferred);
		void handleHeaderResponse (Ref<ResponseHeaderParser> header, boost::system::error_code ec);

		void onChunkHeader (std::uint64_t size, beast::string_view extensions, boost::system::error_code& ev);
		std::size_t onChunkBody (std::uint64_t remain, beast::string_view body, boost::system::error_code& ec);

		void readChunk (Ref<ResponseHeaderParser> header);
		void handleChunkResponse (Ref<ResponseHeaderParser> header, boost::system::error_code ec, std::size_t bytes_tranferred);
	
	private:
		std::function<void (Ref<ResponseParser>, boost::system::error_code, std::size_t)> responseHandler;
		std::function<void (Ref<ResponseHeaderParser>, std::function<void()>)> headerHandler;
		std::function<void (std::uint64_t, beast::string_view)> chunkHeaderHandler;
		std::function<void (std::uint64_t, beast::string_view)> chunkBodyHandler;

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
		tcp::socket& getSocket () { return socket; }

		void start () {
			LOG_FUNCTION_DEBUG;
			acceptRequest ();
		}

	private:
		std::pair<std::string, uint16_t> extractAddressPort (beast::string_view host);

		void acceptRequest ();
		void handleRequest (Ref<RequestParser> request, boost::system::error_code ec);
		// maybe need remade to another handler
		void forwardRemoteResponse (Ref<ResponseParser> response, boost::system::error_code ec, std::size_t bytes_tranferred);
		void handleWrite (Ref<Response> response, boost::system::error_code ec, std::size_t bytes_transferred);


		void headerHandler (Ref<ResponseHeaderParser> header, std::function<void()> callback);
		void chunkHeaderHandler (std::uint64_t size, beast::string_view extensions);
		void chunkBodyHandler (std::uint64_t remain, beast::string_view body);

		void forwardHeader (Ref<ResponseHeader> header, std::function<void (boost::system::error_code, size_t)> handler);
		template<typename T>
		void forwardChunkBody (const T & chunk);

	private:
		beast::flat_buffer buffer;
		tcp::socket socket;
		asio::strand<asio::io_context::executor_type> strand;

		asio::io_context& ioc;

		RemoteStorage remoteSessionStorage;
	};
}