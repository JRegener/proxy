#include "Socket.h"


namespace proxy {

void Socket::destroy () {
	LOG_FUNCTION_DEBUG;

	stop ();
	close ();
}

void Socket::start () {
	LOG_FUNCTION_DEBUG;
	expires ();
	timeout.async_wait (std::bind (&Socket::handleTimeout, this, std::placeholders::_1));
}

void Socket::stop () {
	LOG_FUNCTION_DEBUG;
	timeout.cancel ();
}

void Socket::setTimeout (int64_t seconds) {
	LOG_FUNCTION_DEBUG;
	this->seconds = seconds;
}

void Socket::expires () {
	LOG_FUNCTION_DEBUG;
	timeout.expires_from_now (boost::posix_time::seconds (seconds));
}

tcp::socket& Socket::use (Error& error) {
	LOG_FUNCTION_DEBUG;

	error = Error::None;

	if (!socket.is_open ()) {
		// TODO return error code
		error = Error::SocketClosed;
		return socket;
	}

	expires ();
	start ();
	return socket;
}

boost::system::error_code Socket::close () {
	LOG_FUNCTION_DEBUG;

	boost::system::error_code ec;
	if (!socket.is_open ()) return ec;

	socket.shutdown (asio::socket_base::shutdown_both, ec);
	if (ec) return ec;

	socket.close (ec);
	return ec;
}

void Socket::handleTimeout (const boost::system::error_code& ec) {
	LOG_FUNCTION_DEBUG;

	if (ec) {
		if (asio::error::operation_aborted != ec)
			logBoostError (ec);
		return;
	}

	if (!socket.is_open ()) {
		return;
	}

	std::cout << "Connection success closed" << std::endl;
	if (callback) callback ();

	boost::system::error_code err = close ();
	if (err) {
		std::cout << "Error during close connection" << std::endl;
		logBoostError (err);
		return;
	}

	return;
}

}
