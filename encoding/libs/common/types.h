#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t Byte;
typedef uint16_t Word;
typedef uint32_t DWord;
typedef uint64_t QWord;

typedef int8_t SByte;
typedef int16_t SWord;
typedef int32_t SDWord;
typedef int64_t SQWord;

typedef size_t Size;
typedef uintptr_t UIntPtr;

typedef enum {
  RESULT_OK,
  RESULT_ERROR,
  RESULT_IO_ERROR,
  RESULT_MEMORY_ERROR,
  RESULT_INVALID_ARGUMENT
} Result;

#endif // COMMON_TYPES_H
