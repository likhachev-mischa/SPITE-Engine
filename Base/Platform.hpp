#pragma once
#include <cstdint>

//stolen from Mastering Vulkan

//macros

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array)[0])

#if defined (_MSC_VER)
#define SPITE_INLINE                               inline
#define SPITE_FINLINE                              __forceinline
#define SPITE_DEBUG_BREAK                          __debugbreak();
#define SPITE_DISABLE_WARNING(warning_number)      __pragma( warning( disable : (warning_number) ) )
#define SPITE_CONCAT_OPERATOR(x, y)                x##y
#else
#define SPITE_INLINE                               inline
#define SPITE_FINLINE                              always_inline
#define SPITE_DEBUG_BREAK                          raise(SIGTRAP);
#define SPITE_CONCAT_OPERATOR(x, y)                x y
#endif // MSVC

#define SPITE_STRINGIZE( L )                       #L
#define SPITE_MAKESTRING( L )                      SPITE_STRINGIZE( L )
#define SPITE_CONCAT(x, y)                         SPITE_CONCAT_OPERATOR(x, y)
#define SPITE_LINE_STRING                          SPITE_MAKESTRING( __LINE__ )
#define SPITE_FILELINE(MESSAGE)                    __FILE__ "(" SPITE_LINE_STRING ") : " MESSAGE

//native types
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef long long unsigned sizet;

typedef const char* cstring;

static constexpr u64 U64_MAX = UINT64_MAX;
static constexpr i64 I64_MAX = INT64_MAX;
static constexpr u32 U32_MAX = UINT32_MAX;
static constexpr i32 I32_MAX = INT32_MAX;
static constexpr u16 U16_MAX = UINT16_MAX;
static constexpr i16 I16_MAX = INT16_MAX;
static constexpr u8 U8_MAX = UINT8_MAX;
static constexpr i8 I8_MAX = INT8_MAX;
