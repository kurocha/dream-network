//
//  Service.hpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 1/5/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#pragma once

#include <string>

namespace Dream
{
	namespace Network
	{
		/// A TCP/UDP port number
		typedef int PortNumber;
		
		class Service
		{
		public:
			Service(PortNumber port_number);
			
			Service(const std::string & name) : _name(name) {}
			Service(const char * name) : _name(name) {}
			
			virtual ~Service();
			
			const std::string & name() const { return _name; }
			
		private:
			std::string _name;
		};
	}
}
