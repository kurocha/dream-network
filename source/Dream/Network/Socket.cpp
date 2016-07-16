//
//  Network/Socket.cpp
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 26/10/07.
//  Copyright (c) 2007 Samuel Williams. All rights reserved.
//
//

#include "Socket.hpp"

#include <Dream/Core/System.hpp>

#include <Dream/Events/Loop.hpp>
#include <Dream/Events/Thread.hpp>
#include <Dream/Core/Logger.hpp>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <errno.h>

//#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>

namespace Dream {
	namespace Network {
		using namespace Core::Logging;
		using Core::SystemError;

		Socket::Socket (int s) : _socket(s) {
		}

		Socket::Socket () : _socket(-1) {
		}

		Socket::~Socket () {
			if (is_valid()) {
				close();
			}
		}

		void Socket::open_socket (AddressFamily af, SocketType st, SocketProtocol sp) {
			DREAM_ASSERT(_socket == -1);
			SystemError::reset();

			_socket = ::socket(af, st, sp);
			// std::cerr << "=====> Opening socket " << _socket << std::endl;

			if (_socket == -1)
				SystemError::check(__func__);
		}

		void Socket::open_socket (const Address & a) {
			open_socket(a.address_family(), a.socket_type(), a.socket_protocol());
		}

		Socket::Socket (AddressFamily af, SocketType st, SocketProtocol sp) : _socket(-1) {
			open_socket(af, st, sp);
		}

		void Socket::shutdown (int mode) {
			DREAM_ASSERT(is_valid());
			SystemError::reset();
			
			if (::shutdown(_socket, mode) == -1)
				SystemError::check(__func__);
		}

		void Socket::close () {
			DREAM_ASSERT(is_valid());

			if (::close(_socket) == -1) {
				SystemError::check(__func__);
			}

			_socket = -1;
		}

		FileDescriptor Socket::file_descriptor () const
		{
			return _socket;
		}

		void Socket::set_no_delay_mode (bool mode) {
			int flag = mode ? 1 : 0;

			// IPPROTO_TCP / SOL_SOCKET
			int result = setsockopt(_socket, SOL_SOCKET, TCP_NODELAY, (char*)&flag, sizeof(int));

			if (result == -1) {
				SystemError::check(__func__);
			}
		}
		
		void Socket::set_non_blocking(bool value) {
			int flags = fcntl(_socket, F_GETFL, 0);
			
			if (flags == -1)
				SystemError::check(__func__);
			
			if (value)
				flags |= O_NONBLOCK;
			else
				flags &= ~O_NONBLOCK;
			
			if (fcntl(_socket, F_SETFL, flags) == -1)
				SystemError::check(__func__);
		}

		int Socket::socket_specific_error () const {
			DREAM_ASSERT(is_valid());

			int error = 0;
			socklen_t len = sizeof(error);

			//Get error code specific to this socket
			getsockopt(_socket, SOL_SOCKET, SO_ERROR, &error, &len);

			return error;
		}

		bool Socket::is_valid () const {
			return _socket != -1;
		}

		bool Socket::is_connected () const {
			sockaddr sa;
			socklen_t len = sizeof(sockaddr);

			int err = getpeername(_socket, &sa, &len);

			if (errno == ENOTCONN || err == -1) {
				return false;
			}

			return true;
		}

		void Socket::shutdown_read()
		{
			shutdown(SHUT_RD);
		}

		void Socket::shutdown_write()
		{
			shutdown(SHUT_WR);
		}

		std::size_t Socket::send (const Core::Buffer & buf, std::size_t offset, int flags) {
			DREAM_ASSERT(buf.size() > 0);
			DREAM_ASSERT(offset < buf.size());

			//std::cout << "Sending " << buf.size() << " bytes..." << std::endl;

			ssize_t sz = buf.size() - offset;

			const Byte * data = buf.begin() + offset;

			sz = ::send(_socket, data, sz, flags);

			if (sz == 0)
				throw ConnectionShutdown("write shutdown");

			if (sz == -1) {
				if (errno == ECONNRESET)
					throw ConnectionResetByPeer("write error");

				SystemError::check(__func__);

				sz = 0;
			}

			return sz;
		}

		std::size_t Socket::recv (Core::ResizableBuffer & buf, int flags) {
			DREAM_ASSERT(buf.size() < buf.capacity() && "Please make sure you have reserved space for incoming data");

			//std::cout << "Receiving " << (buf.capacity() - buf.size()) << " bytes..." << std::endl;

			// Find out where we are writing data
			std::size_t offset = buf.size();

			// Firstly, we maximize size in one go
			buf.resize(buf.capacity());

			// We read the size in the buffer
			ssize_t sz = ::recv(_socket, (void*)&buf[offset], buf.size() - offset, flags);

			if (sz == 0)
				throw ConnectionShutdown("read shutdown");

			if (sz == -1) {
				if (errno == ECONNRESET)
					throw ConnectionResetByPeer("read error");

				SystemError::check(__func__);

				sz = 0;
			}

			// We resize to
			buf.resize(offset + sz);

			return sz;
		}

// MARK: -
// MARK: class ServerSocket

		ServerSocket::ServerSocket (const Address &server_address, unsigned listen_count) {
			bind(server_address);
			listen(listen_count);

			set_will_block(false);

			log_debug("Server", this, "starting on address:", server_address.description(), "fd:", file_descriptor());
		}

		ServerSocket::~ServerSocket () {
		}

		void ServerSocket::process_events (Events::Loop * event_loop, Events::Event events)
		{
			if (events & Events::READ_READY) {
				DREAM_ASSERT(!!connection_callback);

				SocketHandleT socket_handle;
				Address address;

				while (accept(socket_handle, address))
					connection_callback(event_loop, this, socket_handle, address);
			}
		}

		void ServerSocket::bind (const Address & na, bool reuse_addr) {
			open_socket(na);

			DREAM_ASSERT(is_valid() && na.is_valid());

			if (reuse_addr) {
				set_reuse_address(true);
			}

			if (::bind(_socket, na.address_data(), na.address_data_size()) == -1) {
				SystemError::check(__func__);
				
				return;
			}

			_bound_address = na;
		}

		void ServerSocket::listen (unsigned n) {
			if (::listen(_socket, n) == -1) {
				SystemError::check(__func__);
			}
		}

		const Address & ServerSocket::bound_address () const
		{
			return _bound_address;
		}

		bool ServerSocket::accept (SocketHandleT & h, Address & na) {
			DREAM_ASSERT(is_valid());

			socklen_t len = sizeof(sockaddr_storage);
			sockaddr_storage ss;
			h = ::accept(_socket, (sockaddr*)&ss, &len);

			if (h == -1) {
				if (errno != EWOULDBLOCK) {
					SystemError::check(__func__);
				}

				return false;
			}

			// Copy address
			na = Address(_bound_address, (sockaddr*)&ss, len);

			return true;
		}

		void ServerSocket::set_reuse_address (bool enabled) {
			DREAM_ASSERT(is_valid());

			int value = (int)enabled;
			
			if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) == -1) {
				SystemError::check(__func__);
			}
			
#ifdef SO_REUSEPORT
			if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(int)) == -1) {
				SystemError::check(__func__);
			}
#endif
		}

// MARK: -
// MARK: class ClientSocket



		ClientSocket::ClientSocket(const SocketHandleT & h, const Address & address) {
			_socket = h;
			_remote_address = address;
		}

		ClientSocket::ClientSocket() {
		}

		ClientSocket::~ClientSocket () {
		}

		const Address & ClientSocket::remote_address () const
		{
			return _remote_address;
		}

		bool ClientSocket::connect (const Address & na) {
			DREAM_ASSERT(!is_valid());

			open_socket(na);

			DREAM_ASSERT(is_valid() && na.is_valid());

			if (::connect(_socket, na.address_data(), na.address_data_size()) == -1) {
				if (errno == ECONNRESET) {
					throw ConnectionResetByPeer("connect error");
				} else {
					SystemError::check(__func__);
				}

				return false;
			}

			_remote_address = na;

			return true;
		}

		bool ClientSocket::connect (const AddressesT & addresses)
		{
			for (auto address : addresses) {
				if (connect(address))
					return true;
			}

			return false;
		}

		void ClientSocket::process_events (Events::Loop * event_loop, Events::Event events)
		{
		}
	}
}
