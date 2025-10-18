#ifndef ARCHIVE_FORMAT_ARCHIVE_FORMAT_H
#define ARCHIVE_FORMAT_ARCHIVE_FORMAT_H

#include <stdbool.h>
#include <stdio.h>

#include "types.h"

#define ARCHIVE_SIGNATURE "LOLKEK"  // 6-байтовая сигнатура
#define ARCHIVE_SIGNATURE_SIZE 6
#define ARCHIVE_VERSION 0  // Версия формата для этого задания

typedef struct
{
  char signature[ARCHIVE_SIGNATURE_SIZE];  // 6 байт
  uint16_t version;                        // 2 байта
  uint64_t original_size;                  // 8 байт
} ArchiveHeader;

#define ARCHIVE_HEADER_SIZE (sizeof(ArchiveHeader))

bool archive_header_is_valid(const ArchiveHeader* header);
void archive_header_init(ArchiveHeader* header, uint64_t original_size);
Result archive_header_write(const ArchiveHeader* header, FILE* file);
Result archive_header_read(ArchiveHeader* header, FILE* file);

#endif  // ARCHIVE_FORMAT_ARCHIVE_FORMAT_H
