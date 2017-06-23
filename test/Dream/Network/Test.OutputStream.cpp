//
//  Test.OutputStream.cpp
//  This file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 23/6/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Dream/Network/OutputStream.hpp>
#include <Buffers/StaticBuffer.hpp>

#include <unistd.h>

namespace Dream
{
	namespace Network
	{
		UnitTest::Suite OutputStreamTestSuite {
			"Dream::Network::OutputStream",
			
			{"it can write buffer to pipe",
				[](UnitTest::Examiner & examiner) {
					FileDescriptor file_descriptors[2];
					auto test = Shared<StaticBuffer>::make("test", false);
					
					::pipe(file_descriptors);
					
					OutputStream _output_stream;
					
					for (std::size_t i = 0; i < 100; i += 1) {
						examiner << "Writing the buffer to the pipe" << std::endl;
						_output_stream.append(test);
						examiner.expect(_output_stream.write_to(file_descriptors[1])) == 4;
						
						Byte buffer[4];
						examiner << "Reading the data from the pipe" << std::endl;
						examiner.expect(::read(file_descriptors[0], buffer, 4)) == 4;
						examiner.expect(std::string(buffer, buffer+4)) == "test";
					}
					
					::close(file_descriptors[0]);
					::close(file_descriptors[1]);
				}
			},
		};
	}
}
