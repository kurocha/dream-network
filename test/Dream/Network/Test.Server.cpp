//
//  Network/ServerTest.cpp
//  This file is part of the "Dream" project, and is released under the MIT license.
//
//  Created by Samuel Williams on 19/12/08.
//  Copyright (c) 2008 Samuel Williams. All rights reserved.
//
//

#include <Dream/Core/Logger.hpp>
#include <Dream/Core/System.hpp>

#include <Dream/Network/Network.hpp>
#include <Dream/Network/Message.hpp>
#include <Dream/Network/Server.hpp>

#include <functional>
#include <future>

#include <Euclid/Numerics/Average.hpp>

#include <UnitTest/UnitTest.hpp>

namespace Dream
{
	namespace Network
	{
		using namespace Core;
		using namespace Logging;
		
		using namespace Events;
		using Euclid::Numerics::Average;
		const unsigned PK_PING = 0xAF;

		Average<TimeT> global_latency;
		std::mutex global_latency_lock, global_output_lock;
		typedef std::lock_guard<std::mutex> scoped_lock;

		class Pinger : public MessageClientSocket {
		protected:
			int _ttl = 50;
			bool _started = false;

			Timer _timer;
			Average<TimeT> _avg;

		public:
			Pinger (const SocketHandleT & h, const Address & a) : MessageClientSocket(h, a) {
				message_received_callback = std::bind(&Pinger::received_message, this);
				send_ping ();
			}

			Pinger () : _ttl(50), _started(false) {
				message_received_callback = std::bind(&Pinger::received_message, this);
			}

			virtual ~Pinger () {
				//{
				//	scoped_lock _s(global_output_lock);
				//	std::cout << "Individual average is: " << (_avg.average() * 1000.0) << "ms" << std::endl;
				//}

				if (_avg.has_samples()) {
					scoped_lock lock(global_latency_lock);
					global_latency.add_samples(_avg);
				}
			}

			virtual void process_events (Loop * runloop, Event events) {
				if (events & Events::WRITE_READY && _started == false) {
					_started = true;
					send_ping();
				}

				MessageClientSocket::process_events(runloop, events);
			}

			void received_message () {
				TimeT total = _timer.time();

				Ref<Message> recv_msg = received_messages().front();
				received_messages().pop();

				if (recv_msg->header()->packet_type == PK_PING) {
					_avg.add_sample(total);
				}

				_ttl -= 1;
				if (_ttl > 0) {
					send_ping();
				} else {
					shutdown();
				}
			}

			void send_ping () {
				Ref<Message> send_msg (new Message);

				send_msg->reset_header();
				send_msg->header()->packet_type = PK_PING;

				send_message(send_msg);

				_timer.reset();
			}
		};

		static void run_efficient_client_process (int k) {
			try {
				AddressesT server_addresses = Address::addresses_for_name("localhost", "1404", SOCK_STREAM);

				Ref<Loop> clients = new Loop;

				// Connect k times.
				for (unsigned i = 0; i < k; i += 1) {
					Ref<Pinger> s (new Pinger);
					s->connect(server_addresses);

					if (s->is_connected()) {
						clients->monitor(s);
					}
				}

				clients->run_forever();
			} catch (Dream::Core::SystemError & error) {
				log_error(error.what());
			}
		}

		class PingPongServer : public Server {
		protected:
			void message_received (MessageClientSocket * client) {
				while (client->received_messages().size()) {
					Ref<Message> msg = client->received_messages().front();
					client->received_messages().pop();

					Ref<Message> pong_msg = new Message;
					pong_msg->reset_header();
					pong_msg->header()->packet_type = PK_PING;

					client->send_message(pong_msg);
				}
			}

			virtual void connection_callback (Loop * event_loop, ServerSocket * server_socket, const SocketHandleT & h, const Address & a)
			{
				Ref<MessageClientSocket> client_socket = new MessageClientSocket(h, a);

				//std::cerr << "Accepted connection " << client_socket << " from " << client_socket->remote_address().description();
				//std::cerr << " (" << client_socket->remote_address().address_family_name() << ")" << std::endl;

				client_socket->message_received_callback = std::bind(&PingPongServer::message_received, this, std::placeholders::_1);

				event_loop->monitor(client_socket);
			}

		public:
			PingPongServer (Ref<Loop> event_loop, const char * service_name, SocketType socket_type) : Server(event_loop)
			{
				bind_to_service(service_name, socket_type);
			}

			virtual ~PingPongServer ()
			{
			}
		};

		UnitTest::Suite LoopTestSuite {
			"Dream::Network::Server",
	
			{"a complete server running with multiple connections",
				[](UnitTest::Examiner & examiner) {
					int k = 100;

					for (int i = 0; i < 2; i++) {
						log("Run", i);

						//global_latency = Numerics::Average<TimeT>();

						Ref<ServerContainer> container(new ServerContainer);

						Ref<Server> server(new PingPongServer(container->event_loop(), "1404", SOCK_STREAM));
						container->start(server);

						std::vector<std::future<void>> children;

						sleep(1);
						children.push_back(std::async(run_efficient_client_process, k));
						children.push_back(std::async(run_efficient_client_process, k));

						sleep(1);
						children.push_back(std::async(run_efficient_client_process, k));
						children.push_back(std::async(run_efficient_client_process, k));

						sleep(1);
						children.push_back(std::async(run_efficient_client_process, k));
						children.push_back(std::async(run_efficient_client_process, k));

						for(auto & thread : children) {
							thread.get();
						}

						container->stop();

						{
							scoped_lock lock(global_latency_lock);
							log("Average latency (whole time):", global_latency.value() * 1000.0, "ms");
						}
					}
				}
			},
		};
	}
}
