#ifndef UINT_H_INCLUDED
#define UINT_H_INCLUDED

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

#define UINT8_COUNT (UINT8_MAX + 1)

#define TOU16(b1, b2) ((b2) << 8 | (b1))

#endif
