#pragma once

namespace spite
{
#ifndef SPITE_TEST
#define SASSERT(condition) if (!(condition)) {SDEBUG_LOG(SPITE_FILELINE("FALSE\n")); SPITE_DEBUG_BREAK}
#if defined(_MSC_VER)
#define SASSERTM(condition,message) if (!(condition)) {SDEBUG_LOG(SPITE_FILELINE(SPITE_CONCAT(message,"\n"))); SPITE_DEBUG_BREAK}
#endif
#else
#define SASSERT(condition) if (!(condition)) {throw std::runtime_error("Assertion failed!");}
#define SASSERTM(condition,message) if (!(condition)) {throw std::runtime_error("Assertion failed!");}
#endif
}
