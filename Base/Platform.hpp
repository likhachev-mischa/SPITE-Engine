#pragma once
#include <cstdint>

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

typedef size_t sizet;

typedef const char* cstring;

static constexpr u64 U64_MAX = UINT64_MAX;
static constexpr i64 I64_MAX = INT64_MAX;
static constexpr u32 U32_MAX = UINT32_MAX;
static constexpr i32 I32_MAX = INT32_MAX;
static constexpr u16 U16_MAX = UINT16_MAX;
static constexpr i16 I16_MAX = INT16_MAX;
static constexpr u8 U8_MAX = UINT8_MAX;
static constexpr i8 I8_MAX = INT8_MAX;
