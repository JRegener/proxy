#pragma once

#include "Proxy.h"

namespace proxy {
	

}

namespace proxy::debug {

class Utils {
public:
	static void printRequest (const RequestParser& request) {
		std::cout << std::endl;
		std::cout << "Request" << std::endl;

		auto message = request.get ();

		std::cout << message.version () << std::endl;
		std::cout << message.target () << std::endl;
		std::cout << message.method () << std::endl;

		std::cout << std::endl;
		for (auto& element : message) {
			std::cout << element.name_string () << ": " << element.value () << std::endl;
		}

		std::cout << std::endl;
		std::cout << message.body () << std::endl;

	}
};

}