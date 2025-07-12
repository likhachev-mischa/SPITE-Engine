#pragma once

#include "Platform.hpp"
#include "Service.hpp"

namespace spite
{
	struct LogService
	{
		//SPITE_DECLARE_SERVICE(LogService)

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

	//made for counting test logs and asserting quantity of logged events
#if defined(SPITE_TEST)

	//struct TestLogService
	//{
	//	//key - string log, value - log's count
	//	eastl::hash_map<std::string, sizet, std::hash<std::string>, std::equal_to<std::string>,
	//	                spite::HeapAllocator> logsLookup;

	//	TestLogService(const spite::HeapAllocator& allocator): logsLookup(allocator)
	//	{
	//	}

	//	inline void printTestLog(const std::string& log)
	//	{
	//		recordTestLog(log);
	//		SDEBUG_LOG(log.c_str())
	//	}

	//	inline void recordTestLog(const std::string& log)
	//	{
	//		if (logsLookup.find(log) == logsLookup.end())
	//		{
	//			logsLookup.emplace(log, 1);
	//		}
	//		else
	//		{
	//			++logsLookup.at(log);
	//		}
	//	}

	//	void dispose()
	//	{
	//		logsLookup.clear();
	//	}

	//	~TestLogService()
	//	{
	//		dispose();
	//	}
	//};

	//inline TestLogService& getTestLoggerInstance()
	//{
	//	static TestLogService instance(getGlobalAllocator());
	//	return instance;
	//}

	////should not be used in debug/release build, only for unit testing
	//inline sizet getTestLogCount(const std::string& log)
	//{
	//	auto& lookup = getTestLoggerInstance().logsLookup;
	//	if (lookup.find(log) == lookup.end())
	//	{
	//		return 0;
	//	}
	//	return lookup.at(log);
	//}

#define STEST_LOG(string) spite::getTestLoggerInstance().printTestLog(string);
#define STEST_LOG_UNPRINTED(string) spite::getTestLoggerInstance().recordTestLog(string);
#define STEST_LOG_FUNC(string) spite::getTestLoggerInstance().printTestLog(std::string("[") + SPITE_FUNCTION + "] " + string);
#define STEST_LOG_UNPRINTED_FUNC(string) spite::getTestLoggerInstance().recordTestLog(std::string("[") + SPITE_FUNCTION + "] " + string);
#else
#define STEST_LOG(string) 
#define STEST_LOG_UNPRINTED(string)
#define STEST_LOG_FUNC(string)
#define STEST_LOG_UNPRINTED_FUNC(string)
#endif
}
