/* rk_type.h - Stub for local IDE (macOS). DO NOT use for actual compilation. */
#pragma once
#include <stdint.h>
#include <stddef.h>

typedef int8_t   RK_S8;
typedef uint8_t  RK_U8;
typedef int16_t  RK_S16;
typedef uint16_t RK_U16;
typedef int32_t  RK_S32;
typedef uint32_t RK_U32;
typedef int64_t  RK_S64;
typedef uint64_t RK_U64;
typedef float    RK_FLOAT;
typedef double   RK_DOUBLE;
typedef void     RK_VOID;
typedef char     RK_CHAR;

typedef RK_S32   RK_HANDLE;
typedef RK_U64   MB_BLK;
typedef RK_U32   MB_POOL;

#define RK_NULL    NULL
#define RK_TRUE    1
#define RK_FALSE   0
#define RK_SUCCESS 0
#define RK_FAILURE (-1)
