#include "compressed_archive_header.h"

#include <stdbool.h>
#include <string.h>

#include "crc32.h"
#include "file.h"
#include "types.h"

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

  if (header->version_major < 1 || header->version_major > 2)
  {
    return false;
  }

  return true;
}

Result compressed_archive_header_init(CompressedArchiveHeader* header,
                                      QWord original_size,
                                      Byte primary_compression,
                                      Byte secondary_compression,
                                      Byte error_correction, DWord flags)
{
  if (header == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
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
  header->primary_tree_model_size = 0;
  header->huffman_tree_size = 0;
  header->arithmetic_model_size = 0;
  header->shannon_tree_size = 0;
  header->secondary_context_size = 0;
  header->rle_context_size = 0;
  header->lz78_context_size = 0;
  header->lz77_context_size = 0;
  header->header_crc = 0;
  header->data_crc = 0;

  if (header->version_major == 1)
  {
    header->huffman_tree_size = 0;
    header->arithmetic_model_size = 0;
    header->shannon_tree_size = 0;
    header->rle_context_size = 0;
    header->lz78_context_size = 0;
    header->lz77_context_size = 0;
  }

  CRC32Table* crc32_table = crc32_table_create();
  if (crc32_table == NULL)
  {
    return RESULT_MEMORY_ERROR;
  }

  Result result = crc32_table_calculate(crc32_table, (const Byte*)header,
                                        COMPRESSED_ARCHIVE_HEADER_SIZE);
  if (result != RESULT_OK)
  {
    crc32_table_destroy(crc32_table);
    return RESULT_ERROR;
  }

  header->header_crc = crc32_table_get_crc32(crc32_table);
  crc32_table_destroy(crc32_table);

  return RESULT_OK;
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
