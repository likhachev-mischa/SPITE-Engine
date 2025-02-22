#pragma once

#if defined (SPITE_TEST)
#include "Logging.hpp"
#include <iostream>
#endif

namespace spite
{
#if defined (SPITE_TEST)
#define SASSERT(condition) if ((condition)){} else {SDEBUG_LOG("Assertion failed!");throw std::runtime_error("Assertion failed!");}
#define SASSERTM(condition,format,...) if ((condition)){} else{SDEBUG_LOG(format,__VA_ARGS__)throw std::runtime_error("Assertion failed!");}
#else
#define SASSERT(condition) if ((condition)){} else {SDEBUG_LOG(SPITE_FILELINE("FALSE\n")); SPITE_DEBUG_BREAK}
#if defined(_MSC_VER)
#define SASSERTM(condition,format,...) if ((condition)){} else{SDEBUG_LOG(SPITE_FILELINE(SPITE_CONCAT(format,"\n")),__VA_ARGS__); SPITE_DEBUG_BREAK}
#endif
#endif
}
