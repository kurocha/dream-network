//
//  InputStream.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 23/6/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#include "InputStream.hpp"

namespace Dream
{
	namespace Network
	{
		InputStream::InputStream(std::size_t buffer_size) : _buffer(buffer_size), _ring_buffer(_buffer)
		{
		}
		
		InputStream::~InputStream()
		{
		}
		
		std::size_t InputStream::available() const
		{
			return _ring_buffer.total_size();
		}
		
		void InputStream::consume(std::size_t amount)
		{
			_ring_buffer.consume(amount);
		}
		
		std::size_t InputStream::read_from(FileDescriptor file_descriptor)
		{
			return _ring_buffer.read_from(file_descriptor);
		}
	}
}
