#include "compressed_archive_format.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define ALL_POSSIBLE_BYTES 256

static uint32_t crc32_table[ALL_POSSIBLE_BYTES];
static bool crc32_initialized = false;

static void init_crc32_table(void)
{
  if (crc32_initialized)
  {
    return;
  }

  const uint32_t polynomial = 0xEDB88320;
  for (uint32_t byte = 0; byte < ALL_POSSIBLE_BYTES; byte++)
  {
    uint32_t current_byte = byte;
    const size_t bits_in_byte = 8;
    for (size_t bit = 0; bit < bits_in_byte; bit++)
    {
      if (current_byte & 1)
      {
        current_byte = polynomial ^ (current_byte >> 1);
      }
      else
      {
        current_byte >>= 1;
      }
    }
    crc32_table[byte] = current_byte;
  }
  crc32_initialized = true;
}

bool compressed_archive_header_is_valid(const CompressedArchiveHeader* header)
{
  if (header == NULL)
  {
    return false;
  }

  if (memcmp(header->signature, COMPRESSED_ARCHIVE_SIGNATURE,
             COMPRESSED_ARCHIVE_SIGNATURE_SIZE) != 0)
  {
    return false;
  }

  if (header->version_major < COMPRESSED_ARCHIVE_VERSION_MAJOR)
  {
    return false;
  }

  return true;
}

void compressed_archive_header_init(CompressedArchiveHeader* header,
                                    uint64_t original_size,
                                    uint8_t primary_compression,
                                    uint8_t secondary_compression,
                                    uint8_t error_correction, uint32_t flags)
{
  if (header == NULL)
  {
    return;
  }

  memcpy(header->signature, COMPRESSED_ARCHIVE_SIGNATURE,
         COMPRESSED_ARCHIVE_SIGNATURE_SIZE);
  header->version_major = COMPRESSED_ARCHIVE_VERSION_MAJOR;
  header->version_minor = COMPRESSED_ARCHIVE_VERSION_MINOR;
  header->original_size = original_size;
  header->primary_compression = primary_compression;
  header->secondary_compression = secondary_compression;
  header->error_correction = error_correction;
  header->flags = flags;
  header->header_size = COMPRESSED_ARCHIVE_HEADER_SIZE;
  header->metadata_size = 0;
  header->compressed_size = 0;
  header->header_crc = 0;
  header->data_crc = 0;

  init_crc32_table();
  header->header_crc =
    calculate_crc32((const Byte*)header, COMPRESSED_ARCHIVE_HEADER_SIZE);
}

Result compressed_archive_header_write(const CompressedArchiveHeader* header,
                                       File* file)
{
  if (header == NULL || file == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  return file_write_bytes(file, (const Byte*)header,
                          COMPRESSED_ARCHIVE_HEADER_SIZE);
}

Result compressed_archive_header_read(CompressedArchiveHeader* header,
                                      File* file)
{
  if (header == NULL || file == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  return file_read_bytes_size(file, (Byte*)header,
                              COMPRESSED_ARCHIVE_HEADER_SIZE);
}

uint32_t calculate_crc32(const Byte* data, Size size)
{
  if (data == NULL || size == 0)
  {
    return 0;
  }

  const uint32_t initial_value = 0xFFFFFFFF;
  const uint32_t least_significant_byte = 0xFF;
  const uint32_t bits_in_byte = 8;

  init_crc32_table();

  uint32_t crc = initial_value;

  for (Size i = 0; i < size; i++)
  {
    crc = crc32_table[(crc ^ data[i]) & least_significant_byte] ^
          (crc >> bits_in_byte);
  }

  return crc ^ initial_value;
}
