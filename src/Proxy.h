#pragma once
#include <iostream>
#include <functional>
#include <future>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#pragma warning (disable: 4996 4083)

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename ... Args>
static constexpr Ref<T> createRef (Args&& ... args) {
	return std::make_shared<T> (std::forward<Args> (args)...);
}

#define LOG_FUNCTION_DEBUG std::cout << __FUNCTION__ <<  std::endl;

namespace proxy {
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
}
