//
//  Network/Server.cpp
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 17/11/07.
//  Copyright (c) 2007 Samuel Williams. All rights reserved.
//
//

#include "Server.hpp"

#include <Dream/Core/Timer.hpp>
#include <Dream/Core/System.hpp>
#include <Dream/Events/Loop.hpp>
#include <Dream/Core/Logger.hpp>

namespace Dream {
	namespace Network {
		using namespace Events;
		using namespace Dream::Core::Logging;
		using std::bind;

		ServerContainer::ServerContainer () : _run(false)
		{
			_event_loop = new Loop;
		}

		ServerContainer::~ServerContainer ()
		{
			stop();
		}

		void ServerContainer::run ()
		{
			_event_loop->run_forever();
		}

		Ref<Loop> ServerContainer::event_loop ()
		{
			return _event_loop;
		}

		void ServerContainer::start (Ref<Server> server) {
			if (!_run) {
				_server = server;

				_run = true;

				std::cerr << "Starting server container..." << std::endl;

				DREAM_ASSERT(!_thread);

				_thread = new std::thread(std::bind(&ServerContainer::run, this));
			}
		}

		void ServerContainer::stop () {
			if (_run) {
				std::cerr << "Stopping server container..." << std::endl;

				// Stop the runloop
				_event_loop->stop();

				_thread->join();
				_thread = NULL;

				_run = false;
			}
		}

		Server::Server (Ref<Loop> event_loop) : _event_loop(event_loop)
		{
		}

		Server::~Server ()
		{
			if (_event_loop) {
				for (auto server_socket : _server_sockets)
				{
					_event_loop->stop_monitoring_file_descriptor(server_socket);
				}
			}
		}

		void Server::bind_to_service (const char * service, SocketType sock_type)
		{
			AddressesT server_addresses = Address::interface_addresses_for_service(service, sock_type);

			for (auto address : server_addresses) {
				Ref<ServerSocket> server_socket(new ServerSocket(address));
				server_socket->connection_callback = std::bind(&Server::connection_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

				_server_sockets.push_back(server_socket);

				_event_loop->monitor(server_socket);
			}
		}
	}
}
