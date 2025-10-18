#include "archive_format.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "types.h"

bool archive_header_is_valid(const ArchiveHeader* header)
{
  if (!header)
  {
    return false;
  }

  if (memcmp(header->signature, ARCHIVE_SIGNATURE, ARCHIVE_SIGNATURE_SIZE) != 0)
  {
    return false;
  }

  if (header->version != ARCHIVE_VERSION)
  {
    return false;
  }

  return true;
}

void archive_header_init(ArchiveHeader* header, uint64_t original_size)
{
  if (!header)
  {
    return;
  }

  memcpy(header->signature, ARCHIVE_SIGNATURE, ARCHIVE_SIGNATURE_SIZE);
  header->version = ARCHIVE_VERSION;
  header->original_size = original_size;
}

Result archive_header_write(const ArchiveHeader* header, FILE* file)
{
  if (!header || !file)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  size_t write_status = fwrite(header, sizeof(ArchiveHeader), 1, file);
  if (write_status != 1)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

Result archive_header_read(ArchiveHeader* header, FILE* file)
{
  if (!header || !file)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  size_t read_status = fread(header, sizeof(ArchiveHeader), 1, file);
  if (read_status != 1)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}
