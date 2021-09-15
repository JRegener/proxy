#include "IoContext.h"

namespace proxy {
	asio::io_context& ioContext () {
		static asio::io_context ioc;
		return ioc;
	}
}