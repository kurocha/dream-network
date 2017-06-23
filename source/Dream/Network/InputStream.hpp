//
//  InputStream.hpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 23/6/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#pragma once

#include "Network.hpp"

#include <Buffers/DynamicBuffer.hpp>
#include <Buffers/RingBuffer.hpp>

namespace Dream
{
	namespace Network
	{
		class InputStream
		{
		public:
			InputStream(std::size_t buffer_size = 1024*8);
			virtual ~InputStream();
			
			// Returns the current buffer from which you can read data from.
			// You may need to call this function multiple times if available() is non-zero.
			Buffers::RingBuffer & buffer() {return _ring_buffer;}
			
			// The amount of data available to be read.
			std::size_t available() const;
			
			// Consume data from the start of the buffer.
			void consume(std::size_t amount);
			
			// Read data from the file descriptor.
			std::size_t read_from(FileDescriptor file_descriptor);
			
		private:
			Buffers::DynamicBuffer _buffer;
			Buffers::RingBuffer _ring_buffer;
		};
	}
}
