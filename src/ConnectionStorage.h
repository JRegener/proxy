#pragma once

#include "Proxy.h"

namespace proxy {

	template<typename T>
	class ConnectionStorage {
	public:
		ConnectionStorage () = default;
		~ConnectionStorage () = default;

		bool exist (const Ref<T>& connection) {
			return storage.find (connection->getHostKey ()) != storage.end ();
		}
		bool exist (HostKey key) {
			return storage.find (key) != storage.end ();
		}

		bool add (Ref<T> connection) {
			HostKey key = connection->getHostKey ();
			if (!exist (key)) {
				storage[key] = connection;
				return true;
			}
			return false;
		}
		void remove (const Ref<T>& connection) {
			HostKey key = connection->getHostKey ();
			storage.erase (key);
		}
		void remove (HostKey key) {
			storage.erase (key);
		}
		
		Ref<T> get (const Ref<T>& connection) {
			return exist (connection) ? storage[connection->getHostKey ()] : nullptr;
		}
		Ref<T> get (HostKey key) {
			return exist (key) ? storage[key] : nullptr;
		}

		void clear () {
			storage.clear ();
		}

		typename std::map<HostKey, Ref<T>>::iterator const begin () { return storage.begin (); }
		typename std::map<HostKey, Ref<T>>::iterator const end () { return storage.end (); }

		typename std::map<HostKey, Ref<T>>::const_iterator const cbegin () { return storage.cbegin (); }
		typename std::map<HostKey, Ref<T>>::const_iterator const cend () { return storage.cend (); }


	private:
		std::map<HostKey, Ref<T>> storage;
	};
}