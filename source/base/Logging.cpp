#include "Logging.hpp"

#include <cstdarg>
#include <cstdio>
#include <Windows.h>

namespace spite
{
	LogService g_logService;

	static constexpr u32 K_STRING_BUFFER_SIZE = 1024 * 1024;
	static char gs_logBuffer[K_STRING_BUFFER_SIZE];

	static void outputConsole(char* log_buffer_)
	{
		printf("%s", log_buffer_);
	}

#if defined(_MSC_VER)
	static void outputVisualStudio(char* log_buffer_)
	{
		OutputDebugStringA(log_buffer_);
	}
#endif

	LogService* LogService::instance()
	{
		return &g_logService;
	}

	void LogService::printFormat(cstring format, ...)
	{
		va_list args;

		va_start(args, format);
#if defined(_MSC_VER)
		vsnprintf_s(gs_logBuffer, ARRAY_SIZE(gs_logBuffer), format, args);
#else
        vsnprintf(log_buffer, ARRAY_SIZE(log_buffer), format, args);
#endif
		gs_logBuffer[ARRAY_SIZE(gs_logBuffer) - 1] = '\0';
		va_end(args);

		outputConsole(gs_logBuffer);
#if defined(_MSC_VER)
		outputVisualStudio(gs_logBuffer);
#endif // _MSC_VER
	}
}
