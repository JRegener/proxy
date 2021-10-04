#include "Socket.h"


namespace proxy {

void Socket::start () {
	LOG_FUNCTION_DEBUG;
	timeout.async_wait (std::bind (&Socket::handleTimeout, this, std::placeholders::_1));
}

void Socket::stop () {
	LOG_FUNCTION_DEBUG;
	timeout.cancel ();
}

void Socket::setTimeout (int64_t seconds) {
	LOG_FUNCTION_DEBUG;

	timeout.expires_from_now (boost::posix_time::seconds (seconds));
}

ErrorConnection Socket::close () {
	LOG_FUNCTION_DEBUG;

	boost::system::error_code ec;
	socket.close (ec);

	if (ec) {
		logBoostError (ec);
		return ErrorConnection::error;
	}

	return ErrorConnection::none;
}

void Socket::handleTimeout (const boost::system::error_code& ec) {
	LOG_FUNCTION_DEBUG;

	if (!ec) {
		ErrorConnection err = close ();
		if (err != ErrorConnection::none) {
			std::cout << "Error during closing connection" << std::endl;
		}
		std::cout << "Connection closed" << std::endl;

		return;
	}

	logBoostError (ec);
}

}
