//
//  Message.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 16/7/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Dream/Network/Message.hpp>

namespace Dream
{
	namespace Network
	{
		struct MsgTest {
			Core::Ordered<uint32_t> a;
			Core::Ordered<uint32_t> b;
			Core::Ordered<uint32_t> c;
		};
		
		UnitTest::Suite MessageTestSuite {
			"Dream::Network::Message",
			
			{"it should have some real tests",
				[](UnitTest::Examiner & examiner) {
					Ref<Message> m1(new Message);

					m1->reset_header();
					m1->header()->packet_type = 0xDEAD;

					MsgTest body;
					body.a = 5;
					body.b = 10;
					body.c = body.a + body.b;

					m1->insert(body);
					m1->update_size();

					examiner << "Body is correct size." << std::endl;
					examiner.expect(m1->data_length()) == sizeof(MsgTest);

					examiner << "Header is complete." << std::endl;
					examiner.check(m1->header_complete());
					
					examiner << "Data is complete." << std::endl;
					examiner.check(m1->data_complete());
				}
			},
		};
	}
}
