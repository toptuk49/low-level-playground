#ifndef ARCHIVE_HEADER_RAW_ARCHIVE_HEADER_H
#define ARCHIVE_HEADER_RAW_ARCHIVE_HEADER_H

#include <stdbool.h>

#include "file.h"
#include "types.h"

#define RAW_ARCHIVE_SIGNATURE "lolkek"
#define RAW_ARCHIVE_SIGNATURE_SIZE 6
#define RAW_ARCHIVE_VERSION 0

typedef struct
{
  char signature[RAW_ARCHIVE_SIGNATURE_SIZE];  // 6 Байт
  uint16_t version;                            // 2 Байта
  uint64_t original_size;                      // 8 Байт
} RawArchiveHeader;

#define RAW_ARCHIVE_HEADER_SIZE (sizeof(RawArchiveHeader))

bool raw_archive_header_is_valid(const RawArchiveHeader* header);
void raw_archive_header_init(RawArchiveHeader* header, uint64_t original_size);
Result raw_archive_header_write(const RawArchiveHeader* header, File* file);
Result raw_archive_header_read(RawArchiveHeader* header, File* file);

#endif  // ARCHIVE_HEADER_RAW_ARCHIVE_HEADER_H
