#pragma once

//#if defined (SPITE_TEST)
#include "Logging.hpp"
#include <iostream>
#include "CallstackDebug.hpp"
//#endif

namespace spite
{
#if defined (SPITE_TEST)
#define SASSERT(condition) if ((condition)){} else {SDEBUG_LOG("Assertion failed!");throw std::runtime_error("Assertion failed!");}
#define SASSERTM(condition,format,...) if ((condition)){} else{SDEBUG_LOG(format,__VA_ARGS__)throw std::runtime_error("Assertion failed!");}
#define SASSERT_FUNC(condition) if ((condition)){} else {SDEBUG_LOG_FUNC("Assertion failed!"); throw std::runtime_error("Assertion failed!");}
#define SASSERTM_FUNC(condition,format,...) if ((condition)){} else{SDEBUG_LOG_FUNC(format,__VA_ARGS__); throw std::runtime_error("Assertion failed!");}
#else
	//#define SASSERT(condition) if ((condition)){} else {SDEBUG_LOG(SPITE_FILELINE("FALSE\n")); SPITE_DEBUG_BREAK}
#define SASSERT(condition) if ((condition)){} else {logCallstack(20,"Assertion stack:\n");SDEBUG_LOG(SPITE_FILELINE_FUNC("FALSE\n")); SPITE_DEBUG_BREAK}
#if defined(_MSC_VER)
	//#define SASSERTM(condition,format,...) if ((condition)){} else{SDEBUG_LOG(SPITE_FILELINE(SPITE_CONCAT(format,"\n")),__VA_ARGS__); SPITE_DEBUG_BREAK}
#define SASSERTM(condition,format,...) if ((condition)){} else{logCallstack(20,"Assertion stack:\n");SDEBUG_LOG(SPITE_FILELINE_FUNC(SPITE_CONCAT(format,"\n")),__VA_ARGS__); SPITE_DEBUG_BREAK}
#endif
#endif
}
