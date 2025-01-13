#pragma once

namespace spite
{
#if defined (SPITE_TEST)
#define SASSERT(condition) if (!(condition)) {throw std::runtime_error("Assertion failed!");}
#define SASSERTM(condition,message) if (!(condition)) {throw std::runtime_error("Assertion failed!");}
#else
#define SASSERT(condition) if ((condition)){} else {SDEBUG_LOG(SPITE_FILELINE("FALSE\n")); SPITE_DEBUG_BREAK}
#if defined(_MSC_VER)
#define SASSERTM(condition,format,...) if ((condition)){} else{SDEBUG_LOG(SPITE_FILELINE(SPITE_CONCAT(format,"\n")),__VA_ARGS__); SPITE_DEBUG_BREAK}
#endif
#endif
}
