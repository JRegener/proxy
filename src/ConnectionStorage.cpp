#include "ClientStorage.h"

namespace proxy {
	template<typename T>
	bool ConnectionStorage<T>::exist (const Ref<T> & connection) {
		return storage.find (connection->getConnectionId ()) != storage.end ();
	}

	template<typename T>
	bool ConnectionStorage<T>::exist (ConnectionId id) {
		return storage.find (id) != storage.end ();
	}

	template<typename T>
	bool ConnectionStorage<T>::add (Ref<T> connection) {
		ConnectionId id = connection->getConnectionId ();
		if (!exist (id)) {
			storage[id] = connection;
			return true;
		}
		return false;
	}

	template<typename T>
	bool ConnectionStorage<T>::remove (const Ref<T> & connection) {
		ConnectionId id = connection->getConnectionId ();
		if (exist (id)) {
			storage.erase (id);
			return true;
		}
		return false;
	}

	template<typename T>
	bool ConnectionStorage<T>::remove (ConnectionId id) {
		if (exist (id)) {
			storage.erase (id);
			return true;
		}
		return false;
	}

	template<typename T>
	Ref<T> ConnectionStorage<T>::get (const Ref<T> & connection) {
		return exist (connection) ? storage[connection->getConnectionId ()] : nullptr;
	}

	template<typename T>
	Ref<T> ConnectionStorage<T>::get (ConnectionId id) {
		return exist (id) ? storage[id] : nullptr;
	}

}