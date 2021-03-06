//
//  Address.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 16/7/2016.
//  Copyright, 2016, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Dream/Network/Address.hpp>
#include <Dream/Core/Logger.hpp>

namespace Dream
{
	namespace Network
	{
		using namespace Core::Logging;
		
		UnitTest::Suite AddressTestSuite {
			"Dream::Network::Address",
			
			{"it should resolve interface addresses",
				[](UnitTest::Examiner & examiner) {
					AddressesT addrs1 = Address::interface_addresses_for(1024, SOCK_STREAM);
					
					examiner << "Interface addresses available.";
					examiner.expect(addrs1.size()) > 0;

					bool found_ipv4AddressFamily;
					for(auto & a : addrs1)
					{
						if (a.address_family() == AF_INET)
							found_ipv4AddressFamily = true;
					}

					examiner << "IPv4 address was present.";
					examiner.check(found_ipv4AddressFamily);

					examiner.expect([&](){
						Address::addresses_for_name("localhost", "ThisServiceDoesNotExist", SOCK_STREAM);
					}).to_throw<AddressResolutionError>();

					AddressesT addrs2 = Address::addresses_for_name("localhost", "http", SOCK_STREAM);
					
					examiner << "Host addresses available." << std::endl;
					examiner.expect(addrs2.size()) > 0;
					
					AddressesT addrs3 = Address::addresses_for_uri(Core::URI("http://localhost"));
					
					examiner << "Host addresses available" << std::endl;
					examiner.expect(addrs3.size()) > 0;
				}
			},
		};
	}
}
