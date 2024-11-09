#pragma once

namespace spite
{
#define SASSERT(condition) if (!(condition)) {SDEBUG_LOG(SPITE_FILELINE("FALSE\n")); SPITE_DEBUG_BREAK}
#if defined(_MSC_VER)
#define SASSERTM(condition,message) if (!(condition)) {SDEBUG_LOG(SPITE_FILELINE(SPITE_CONCAT(message,"\n"))); SPITE_DEBUG_BREAK}
#endif

}
