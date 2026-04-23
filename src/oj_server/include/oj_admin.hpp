#pragma once

#include <string>
#include <httplib.h>
#include "oj_control.hpp"

namespace ns_admin
{
	class AdminServer
	{
	public:
		AdminServer();

		void RegisterRoutes(httplib::Server& svr);
		bool Start(const std::string& host = "0.0.0.0", int port = 8090);

	private:
		ns_control::Control _ctl;
	};
}

