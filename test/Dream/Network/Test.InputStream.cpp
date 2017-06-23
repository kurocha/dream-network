//
//  Test.InputStream.cpp
//  This file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 23/6/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Dream/Network/InputStream.hpp>

#include <unistd.h>

namespace Dream
{
	namespace Network
	{
		UnitTest::Suite InputStreamTestSuite {
			"Dream::Network::InputStream",
			
			{"it can read data from pipe",
				[](UnitTest::Examiner & examiner) {
					FileDescriptor file_descriptors[2];
					
					::pipe(file_descriptors);
					
					InputStream _input_stream;
					
					::write(file_descriptors[1], "Test", 4);
					
					_input_stream.read_from(file_descriptors[0]);
					
					examiner.expect(_input_stream.available()) == 4;
					
					::close(file_descriptors[0]);
					::close(file_descriptors[1]);
				}
			},
		};
	}
}
