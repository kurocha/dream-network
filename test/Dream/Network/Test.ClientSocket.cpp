//
//  ClientSocket.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 16/7/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Dream/Network/Socket.hpp>

namespace Dream
{
	namespace Network
	{
		bool global_message_sent;
		std::size_t global_message_length_sent, global_message_length_received;
		bool global_client_connected;
		bool global_message_received;
		
		const std::string global_message("Hello World!");
		
		class TestClientSocket : public ClientSocket {
		public:
			TestClientSocket (const SocketHandleT & h, const Address & address) : ClientSocket(h, address)
			{
				Core::StaticBuffer buf = Core::StaticBuffer::for_cstring(global_message.c_str(), false);

				std::cerr << "Sending message from " << this << "..." << std::endl;

				global_message_length_sent = send(buf);

				std::cerr << global_message_length_sent << " bytes sent" << std::endl;
				global_message_sent = true;
			}

			TestClientSocket ()
			{
			}

			virtual void process_events(Events::Loop * event_loop, Events::Event events)
			{
				if (events & Events::READ_READY) {
					Core::DynamicBuffer buf(1024, true);

					recv(buf);

					global_message_received = true;
					global_message_length_received = buf.size();

					std::string incominglobal_message(buf.begin(), buf.end());

					std::cerr << "Message received by " << this << " fd " << this->file_descriptor() << " : " << incominglobal_message << std::endl;

					global_message_received = (global_message == incominglobal_message);

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
					std::cerr << "Test server has received connection..." << std::endl;
					global_client_connected = true;

					SocketHandleT h;
					Address a;
					if (accept(h, a))
						_test_socket = new TestClientSocket(h, a);

					event_loop->monitor(_test_socket);

					event_loop->stop_monitoring_file_descriptor(this);
				} else {
					std::cerr << "More than one connection received!" << std::endl;
				}
			}

			TestServerSocket (const Address &server_address, unsigned listen_count = 100) : ServerSocket(server_address, listen_count)
			{
			}

			virtual ~TestServerSocket ()
			{
				std::cerr << "Server shutting down..." << std::endl;
			}
		};
		
		static void stop_callback (Events::Loop * event_loop, Events::TimerSource *, Events::Event event)
		{
			std::cerr << "Stopping test" << std::endl;
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
						Address localhost = Address::interface_addresses_for_port(2000, SOCK_STREAM)[0];
						std::cerr << "Initializing server..." << std::endl;
						Ref<TestServerSocket> server_socket = new TestServerSocket(localhost);
						std::cerr << "Initializing client..." << std::endl;
						Ref<TestClientSocket> client_socket = new TestClientSocket;

						std::cerr << "Connecting to server..." << std::endl;
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
				}
			},
		};
	}
}
