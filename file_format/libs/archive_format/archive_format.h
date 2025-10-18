#ifndef ARCHIVE_FORMAT_ARCHIVE_FORMAT_H
#define ARCHIVE_FORMAT_ARCHIVE_FORMAT_H

#include <stdbool.h>

#include "file.h"
#include "types.h"

#define ARCHIVE_SIGNATURE "lolkek"
#define ARCHIVE_SIGNATURE_SIZE 6
#define ARCHIVE_VERSION 0

typedef struct
{
  char signature[ARCHIVE_SIGNATURE_SIZE];  // 6 Байт
  uint16_t version;                        // 2 Байта
  uint64_t original_size;                  // 8 Байт
} ArchiveHeader;

#define ARCHIVE_HEADER_SIZE (sizeof(ArchiveHeader))

bool archive_header_is_valid(const ArchiveHeader* header);
void archive_header_init(ArchiveHeader* header, uint64_t original_size);
Result archive_header_write(const ArchiveHeader* header, File* file);
Result archive_header_read(ArchiveHeader* header, File* file);

#endif  // ARCHIVE_FORMAT_ARCHIVE_FORMAT_H
