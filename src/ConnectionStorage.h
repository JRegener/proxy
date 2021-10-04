#pragma once

#include "Proxy.h"

namespace proxy {

	template<typename T>
	class ConnectionStorage {
	public:
		ConnectionStorage () = default;
		~ConnectionStorage () = default;

		bool exist (const Ref<T>& connection) {
			return storage.find (connection->getConnectionId ()) != storage.end ();
		}
		bool exist (ConnectionId id) {
			return storage.find (id) != storage.end ();
		}

		bool add (Ref<T> connection) {
			ConnectionId id = connection->getConnectionId ();
			if (!exist (id)) {
				storage[id] = connection;
				return true;
			}
			return false;
		}
		bool remove (const Ref<T>& connection) {
			ConnectionId id = connection->getConnectionId ();
			if (exist (id)) {
				storage.erase (id);
				return true;
			}
			return false;
		}
		bool remove (ConnectionId id) {
			if (exist (id)) {
				storage.erase (id);
				return true;
			}
			return false;
		}
		
		Ref<T> get (const Ref<T>& connection) {
			return exist (connection) ? storage[connection->getConnectionId ()] : nullptr;
		}
		Ref<T> get (ConnectionId id) {
			return exist (id) ? storage[id] : nullptr;
		}

		typename std::unordered_map<ConnectionId, Ref<T>>::const_iterator const cbegin () { return storage.cbegin (); }
		typename std::unordered_map<ConnectionId, Ref<T>>::const_iterator const cend () { return storage.cend (); }


	private:
		std::unordered_map<ConnectionId, Ref<T>> storage;
	};
}