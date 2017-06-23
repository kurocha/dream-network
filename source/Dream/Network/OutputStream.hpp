//
//  OutputStream.hpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 23/6/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#pragma once

#include <deque>

#include "Network.hpp"

#include <Buffers/Buffer.hpp>

namespace Dream
{
	namespace Network
	{
		using namespace Buffers;
		
		class OutputStream
		{
		public:
			OutputStream();
			virtual ~OutputStream();
			
			// Append a buffer for writing.
			void append(Shared<Buffer> buffer)
			{
				_buffers.push_back(buffer);
			}
			
			std::size_t write_to(FileDescriptor file_descriptor);
			
		private:
			std::deque<Shared<Buffer>> _buffers;
			std::size_t _offset = 0;
			
		};
	}
}
