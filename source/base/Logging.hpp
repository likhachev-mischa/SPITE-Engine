#pragma once

#include "Platform.hpp"
#include "Service.hpp"

namespace spite
{
	struct LogService final : public Service
	{
		SPITE_DECLARE_SERVICE(LogService)
		void printFormat(cstring format, ...);
	};

 #if defined(_MSC_VER)
 #define SDEBUG_LOG(format,...) spite::LogService::instance()->printFormat(format,__VA_ARGS__);
 #else
 #define SDEBUG_LOG(format,...) spite::LogService::instance()->printFormat(format,## __VA_ARGS__);
 #endif
}
