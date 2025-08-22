#pragma once

#include "Platform.hpp"

namespace spite
{
	struct LogService
	{
		static LogService* instance();

		void printFormat(cstring format, ...);
	};


#if defined(_MSC_VER)
#define SDEBUG_LOG(format, ...) spite::LogService::instance()->printFormat(format,## __VA_ARGS__);
#define SDEBUG_LOG_FUNC(format, ...) spite::LogService::instance()->printFormat("[%s] " format, SPITE_FUNCTION,## __VA_ARGS__);
#else
 #define SDEBUG_LOG(format,...) spite::LogService::instance()->printFormat(format,## __VA_ARGS__);
 #define SDEBUG_LOG_FUNC(format,...) spite::LogService::instance()->printFormat("[%s] " format, SPITE_FUNCTION, ## __VA_ARGS__);
#endif
}
