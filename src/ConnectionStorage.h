#pragma once

#include "Proxy.h"

namespace proxy {

	template<typename T>
	class ConnectionStorage {
	public:
		ConnectionStorage () = default;
		~ConnectionStorage () = default;

		bool exist (const Ref<T>& connection);
		bool exist (ConnectionId id);

		bool add (Ref<T> connection);
		bool remove (const Ref<T>& connection);
		bool remove (ConnectionId id);
		
		Ref<T> get (const Ref<T>& connection);
		Ref<T> get (ConnectionId id);

		std::unordered_map<ConnectionId, Ref<T>>::const_iterator const cbegin () { return storage.cbegin (); }
		std::unordered_map<ConnectionId, Ref<T>>::const_iterator const cend () { return storage.cend (); }


	private:
		std::unordered_map<ConnectionId, Ref<T>> storage;
	};
}