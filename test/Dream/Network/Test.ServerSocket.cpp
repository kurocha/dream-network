//
//  ServerSocket.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 16/7/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Dream/Network/Server.hpp>
#include <Dream/Core/Logger.hpp>

namespace Dream
{
	namespace Network
	{
		using namespace Core::Logging;
		
		int global_message_received_count;
		
		class TestServerClientSocket : public ClientSocket {
		public:
			TestServerClientSocket (const SocketHandleT & h, const Address & address) : ClientSocket(h, address)
			{
			}

			virtual void process_events(Events::Loop * event_loop, Events::Event events)
			{
				if (events & Events::READ_READY) {
					Buffers::DynamicBuffer buffer(1024, true);

					recv(buffer);

					std::string incoming_message(buffer.begin(), buffer.end());

					global_message_received_count += 1;

					log("Message received by", this, "fd:", this->file_descriptor(), "message:", incoming_message);

					event_loop->stop_monitoring_file_descriptor(this);
				}
			}
		};

		class TestServer : public Server {
		protected:
			// (const SocketHandleT & h, const Address & address) : ClientSocket(h, address)
			virtual void connection_callback (Events::Loop * event_loop, ServerSocket * server_socket, const SocketHandleT & h, const Address & a)
			{
				Ref<ClientSocket> client_socket(new TestServerClientSocket(h, a));

				log(
					"Accepted connection:", client_socket, 
					"from:", client_socket->remote_address().description(),
					"address family:", client_socket->remote_address().address_family_name()
				);

				event_loop->monitor(client_socket);
			}

		public:
			TestServer (Ref<Events::Loop> event_loop, const Service & service, SocketType socket_type) : Server(event_loop)
			{
				bind_to_service(service, socket_type);
			}

			virtual ~TestServer ()
			{
			}
		};

		Ref<Events::TimerSource> global_timers[3];

		static void stop_timers_callback (Events::Loop * event_loop, Events::TimerSource *, Events::Event event)
		{
			log("Stoping connection timers...");

			global_timers[0]->cancel();
			global_timers[1]->cancel();
			global_timers[2]->cancel();
		}

		static void stop_callback (Events::Loop * event_loop, Events::TimerSource *, Events::Event event)
		{
			log("Stopping test");
			event_loop->stop();
		}

		int global_message_sent_count;
		int global_address_index;
		AddressesT global_connect_addresses;
		static void connect_callback (Events::Loop * event_loop, Events::TimerSource *, Events::Event event)
		{
			Ref<ClientSocket> test_connection(new ClientSocket);

			test_connection->connect(global_connect_addresses[global_address_index++ % global_connect_addresses.size()]);

			Buffers::StaticBuffer buffer("Hello World?", false);

			global_message_sent_count += 1;
			test_connection->send(buffer);

			test_connection->close();
		}
		
		UnitTest::Suite ServerSocketTestSuite {
			"Dream::Network::ServerSocket",
			
			{"it should connect and send messages",
				[](UnitTest::Examiner & examiner) {
					Ref<Events::Loop> event_loop = new Events::Loop;
					Ref<TestServer> server = new TestServer(event_loop, "7979", SOCK_STREAM);

					global_address_index = 0;
					global_message_received_count = 0;
					global_message_sent_count = 0;

					global_connect_addresses = Address::addresses_for_name("localhost", "7979", SOCK_STREAM);

					event_loop->schedule_timer(new Events::TimerSource(stop_timers_callback, 0.4));
					event_loop->schedule_timer(new Events::TimerSource(stop_callback, 0.5));

					global_timers[0] = new Events::TimerSource(connect_callback, 0.05, true);
					event_loop->schedule_timer(global_timers[0]);

					global_timers[1] = new Events::TimerSource(connect_callback, 0.1, true);
					event_loop->schedule_timer(global_timers[1]);

					global_timers[2] = new Events::TimerSource(connect_callback, 0.11, true);
					event_loop->schedule_timer(global_timers[2]);

					event_loop->run_forever();

					examiner << "Messages sent.";
					examiner.expect(global_message_sent_count) >= 1;
					
					examiner << "Messages received.";
					examiner.expect(global_message_sent_count) == global_message_received_count;
				}
			},
		};
	}
}
