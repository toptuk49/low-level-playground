#ifndef ARCHIVE_READER_COMPRESSED_ARCHIVE_READER_H
#define ARCHIVE_READER_COMPRESSED_ARCHIVE_READER_H

#include "types.h"

typedef struct CompressedArchiveReader CompressedArchiveReader;

CompressedArchiveReader* compressed_archive_reader_create(
  const char* input_filename);
void compressed_archive_reader_destroy(CompressedArchiveReader* self);

Result compressed_archive_reader_extract_all(CompressedArchiveReader* self,
                                             const char* output_path);
Result compressed_archive_reader_extract_file(CompressedArchiveReader* self,
                                              uint32_t file_index,
                                              const char* output_path);

uint32_t compressed_archive_reader_get_file_count(
  const CompressedArchiveReader* self);
const char* compressed_archive_reader_get_filename(
  const CompressedArchiveReader* self, uint32_t index);

#endif  // ARCHIVE_READER_COMPRESSED_ARCHIVE_READER_H
