//
//  OutputStream.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 23/6/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#include "OutputStream.hpp"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

namespace Dream
{
	namespace Network
	{
		OutputStream::OutputStream()
		{
		}
		
		OutputStream::~OutputStream()
		{
		}
		
		inline void * iov_base_pointer(const Byte * base)
		{
			return const_cast<void *>(
				static_cast<const void *>(base)
			);
		}
		
		std::size_t OutputStream::write_to(FileDescriptor file_descriptor)
		{
			// If there is nothing to write, do nothing!
			if (_buffers.empty())
				return 0;
			
			struct iovec iov[_buffers.size()];
			
			// The first write buffer requires special attention due to _offset.
			iov[0].iov_base = iov_base_pointer(_buffers[0]->begin() + _offset);
			iov[0].iov_len = _buffers[0]->size() - _offset;
			
			for (std::size_t i = 1; i < _buffers.size(); i += 1) {
				iov[i].iov_base = iov_base_pointer(_buffers[i]->begin());
				iov[i].iov_len = _buffers[i]->size();
			}
			
			auto result = ::writev(file_descriptor, iov, _buffers.size());
			
			if (result > 0) {
				auto written = result;
				
				for (std::size_t i = 0; i < _buffers.size(); i += 1) {
					auto buffer_size = iov[i].iov_len;
					
					// Did the entire buffer get written?
					if (written > buffer_size) {
						// If so, we remove it.
						written -= buffer_size;
						_buffers.pop_front();
					} else {
						// Otherwise, we record that it was only partially written.
						if (i == 0)
							// Need to handle first buffer carefully.
							_offset += written;
						else
							_offset = written;
						
						break;
					}
				}
			}
			
			// How many bytes were written.
			return result;
		}
	}
}
