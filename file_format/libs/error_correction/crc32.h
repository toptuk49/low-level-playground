#ifndef ERROR_CORRECTION_CRC32_H
#define ERROR_CORRECTION_CRC32_H

#include <inttypes.h>

#include "types.h"

typedef struct CRC32Table CRC32Table;

CRC32Table* crc32_table_create();
void crc32_table_destroy(CRC32Table* self);

Result crc32_table_calculate(CRC32Table* self, const Byte* data, Size size);
uint32_t crc32_table_get_crc32(CRC32Table* self);

#endif  // ERROR_CORRECTION_CRC32_H
