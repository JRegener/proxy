#pragma once

#include "Proxy.h"


namespace proxy {

class HttpError {
public:
	template<typename Body>
	static Response errorResponse (beast::http::status status, std::string_view message,
								   beast::http::request<Body>& req) {
		Response resp (status, req.version ());
		resp.set (beast::http::field::via, "My Proxy");
		resp.set (beast::http::field::content_type, "text/html");
		resp.keep_alive (req.keep_alive ());
		resp.body () = std::string (message);
		resp.prepare_payload ();
		return resp;
	}

	static Response errorResponse (beast::http::status status, std::string_view message,
								   unsigned version, bool keepAlive) {
		Response resp (status, version);
		resp.set (beast::http::field::via, "My Proxy");
		resp.set (beast::http::field::content_type, "text/html");
		resp.keep_alive (keepAlive);
		resp.body () = std::string (message);
		resp.prepare_payload ();
		return resp;
	}

	template<typename Body>
	static Response createErrorResponse (const boost::system::error_code& ec, std::string_view message,
										 beast::http::request<Body>& req) {
		switch (ec.value ()) {
			case asio::error::access_denied:
				// TODO:!!!!!
			default:
				return errorResponse (beast::http::status::internal_server_error,
									  message, req);
		}
	}

	static Response createErrorResponse (const boost::system::error_code& ec, std::string_view message,
										 unsigned version, bool keepAlive) {
		switch (ec.value ()) {
			case asio::error::access_denied:
				// TODO:!!!!!
			default:
				return errorResponse (beast::http::status::internal_server_error,
									  message, version, keepAlive);
		}
	}
};

}