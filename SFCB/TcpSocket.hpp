#ifndef TCPSOCKET_HPP
#define TCPSOCKET_HPP

#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <functional>
#include <algorithm>
#include <vector>

#include "NetworkBase.hpp"
#include "Callback.hpp"

namespace sfcb {
	class TcpSocket
	: public sf::NonCopyable {
	private:
		Callback<const buffer_t&> m_onDataReceived;
		Callback<SocketStatus> m_onError;
		Callback<TcpSocket&> m_onConnected;

		static std::vector<TcpSocket*> sockets;
		sf::TcpSocket m_socket;

		bool m_connecting = false;

	public:
		TcpSocket() {
			m_socket.setBlocking(false);
			sockets.push_back(this);
		}
		~TcpSocket() {
			auto it = std::find(sockets.begin(), sockets.end(), this);
			sockets.erase(it);
		}

		sf::IpAddress getRemoteAddress() const {
			return this->m_socket.getRemoteAddress();
		}

		unsigned short getRemotePort() const {
			return this->m_socket.getRemotePort();
		}

		unsigned short getLocalPort() const {
			return this->m_socket.getLocalPort();
		}

		void connect(const sf::IpAddress& remoteAddress, const unsigned short remotePort, sf::Time timeout = sf::Time::Zero) {
			auto status = this->m_socket.connect(remoteAddress, remotePort, timeout);

			this->m_connecting = true;

			if(status != SocketStatus::Done
			&& status != SocketStatus::NotReady)
				this->m_onError(status);
		}

		void disconnect() {
			this->m_socket.disconnect();
		}

		size_t send(const buffer_t& buffer) {
			size_t sent;
			auto status = this->m_socket.send(buffer.data(), buffer.size(), sent);

			if(status != SocketStatus::Done) {
				this->m_onError(status);
			}

			return sent;
		}

		template<typename func_t, typename ... args_t>
		void onDataReceived(func_t func, const args_t& ... args) {
			this->m_onDataReceived.set(func, args ...);
		}

		template<typename func_t, typename ... args_t>
		void onError(func_t func, const args_t& ... args) {
			this->m_onError.set(func, args ...);
		}

		template<typename func_t, typename ... args_t>
		void onConnected(func_t func, const args_t& ... args) {
			this->m_onConnected.set(func, args ...);
		}

		static void handleCallbacks() {
			for(auto* ptr: sockets) {
				auto& socket = ptr->m_socket;

				SocketStatus status;
				buffer_t data;

				do {
					char buffer[1024];
					std::size_t received = 0;

					status = socket.receive(buffer, sizeof(buffer), received);

					for(auto i = 0u; i < received; ++i) {
						data.push_back(buffer[i]);
					}
				} while(status == SocketStatus::Partial);

				if(status == SocketStatus::Done) {
					ptr->m_onDataReceived(data);
				} else if(status != SocketStatus::NotReady) {
					ptr->m_onError(status);
				}

				if(ptr->m_connecting && socket.getRemotePort()) {
					ptr->m_connecting = false;
					ptr->m_onConnected(*ptr);
				}
			}
		}

		friend class TcpListener;
	};

	std::vector<TcpSocket*> TcpSocket::sockets;
}

#endif // TCPSOCKET_HPP
