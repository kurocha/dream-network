//
//  Service.cpp
//  File file is part of the "Dream" project and released under the MIT License.
//
//  Created by Samuel Williams on 1/5/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#include "Service.hpp"

#include <sstream>

namespace Dream
{
	namespace Network
	{
		Service::Service(PortNumber port_number)
		{
			std::stringstream s;

			s << port_number;
			
			_name = s.str();
		}
		
		Service::~Service()
		{
		}
	}
}
