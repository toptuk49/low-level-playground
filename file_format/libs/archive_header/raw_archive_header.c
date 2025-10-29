#include "raw_archive_header.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "file.h"
#include "types.h"

bool raw_archive_header_is_valid(const RawArchiveHeader* header)
{
  if (!header)
  {
    return false;
  }

  if (memcmp(header->signature, RAW_ARCHIVE_SIGNATURE,
             RAW_ARCHIVE_SIGNATURE_SIZE) != 0)
  {
    return false;
  }

  if (header->version != RAW_ARCHIVE_VERSION)
  {
    return false;
  }

  return true;
}

void raw_archive_header_init(RawArchiveHeader* header, QWord original_size)
{
  if (!header)
  {
    return;
  }

  memcpy(header->signature, RAW_ARCHIVE_SIGNATURE, RAW_ARCHIVE_SIGNATURE_SIZE);
  header->version = RAW_ARCHIVE_VERSION;
  header->original_size = original_size;
}

Result raw_archive_header_write(const RawArchiveHeader* header, File* file)
{
  if (!header || !file)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("Сигнатура: %s\n", header->signature);
  printf("Версия: %d\n", header->version);
  printf("Исходный размер: %" PRIu64 "\n", header->original_size);

  return file_write_bytes(file, (const Byte*)header, sizeof(RawArchiveHeader));
}

Result raw_archive_header_read(RawArchiveHeader* header, File* file)
{
  if (!header || !file)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  return file_read_bytes_size(file, (Byte*)header, sizeof(RawArchiveHeader));
}
