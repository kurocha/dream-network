//
//  ClientSocket.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 16/7/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Dream/Network/Socket.hpp>
#include <Dream/Core/Logger.hpp>

namespace Dream
{
	namespace Network
	{
		using namespace Core::Logging;
		
		bool global_message_sent;
		std::size_t global_message_length_sent, global_message_length_received;
		bool global_client_connected;
		bool global_message_received;
		
		const std::string global_message("Hello World!");
		std::string global_incoming_message;
		
		class TestClientSocket : public ClientSocket {
		public:
			TestClientSocket (const SocketHandleT & h, const Address & address) : ClientSocket(h, address)
			{
				Buffers::StaticBuffer buffer(global_message.data(), false);

				log("Sending", buffer.size(), "byte message from:", this, "...");

				global_message_length_sent = send(buffer);

				log(global_message_length_sent, "bytes sent");
				global_message_sent = true;
			}

			TestClientSocket ()
			{
			}

			virtual void process_events(Events::Loop * event_loop, Events::Event events)
			{
				if (events & Events::READ_READY) {
					Core::DynamicBuffer buffer(1024, true);

					recv(buffer);

					global_message_received = true;
					
					global_message_length_received = buffer.size();

					global_incoming_message = std::string(buffer.begin(), buffer.end());

					log("Message received by", this, "fd:", this->file_descriptor(), "message:", global_incoming_message);

					event_loop->stop_monitoring_file_descriptor(this);
				}
			}
		};

		class TestServerSocket : public ServerSocket {
			Ref<TestClientSocket> _test_socket;

		public:
			virtual void process_events(Events::Loop * event_loop, Events::Event events)
			{
				if (events & Events::READ_READY & !_test_socket) {
					log("Test server has received connection...");
					global_client_connected = true;

					SocketHandleT h;
					Address a;
					if (accept(h, a))
						_test_socket = new TestClientSocket(h, a);

					event_loop->monitor(_test_socket);

					event_loop->stop_monitoring_file_descriptor(this);
				} else {
					log("More than one connection received!");
				}
			}

			TestServerSocket (const Address &server_address, unsigned listen_count = 100) : ServerSocket(server_address, listen_count)
			{
			}

			virtual ~TestServerSocket ()
			{
				log("Server shutting down...");
			}
		};
		
		static void stop_callback (Events::Loop * event_loop, Events::TimerSource *, Events::Event event)
		{
			log("Stopping test");
			event_loop->stop();
		}
		
		UnitTest::Suite ClientSocketTestSuite {
			"Dream::Network::ClientSocket",
			
			{"it should send data between sockets",
				[](UnitTest::Examiner & examiner) {
					global_client_connected = global_message_sent = global_message_received = false;
					global_message_length_sent = global_message_length_received = 0;

					Ref<Events::Loop> event_loop = new Events::Loop;

					// This is a very simple example of a network server listening on a single port.
					// This server can only accept one connection

					{
						Address localhost = Address::interface_addresses_for(2000, SOCK_STREAM)[0];
						log("Initializing server...");
						Ref<TestServerSocket> server_socket = new TestServerSocket(localhost);
						log("Initializing client...");
						Ref<TestClientSocket> client_socket = new TestClientSocket;

						log("Connecting to server...");
						client_socket->connect(localhost);

						event_loop->monitor(server_socket);
						event_loop->monitor(client_socket);
						event_loop->schedule_timer(new Events::TimerSource(stop_callback, 1));
					}

					event_loop->run_forever();
					event_loop = NULL;

					examiner << "Client connected.";
					examiner.check(global_client_connected);
					
					examiner << "Message sent.";
					examiner.check(global_message_sent);
					
					examiner << "Message length is correct.";
					examiner.expect(global_message_length_sent) == global_message_length_received;
					
					examiner << "Message received.";
					examiner.check(global_message_received);
					examiner.expect(global_incoming_message) == global_message;
				}
			},
		};
	}
}
