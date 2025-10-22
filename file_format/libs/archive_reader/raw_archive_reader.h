#ifndef ARCHIVE_READER_RAW_ARCHIVE_READER_H
#define ARCHIVE_READER_RAW_ARCHIVE_READER_H

#include "types.h"

typedef struct RawArchiveReader RawArchiveReader;

RawArchiveReader* raw_archive_reader_create(const char* input_filename);
void raw_archive_reader_destroy(RawArchiveReader* self);

Result raw_archive_reader_extract_all(RawArchiveReader* self,
                                      const char* output_path);
Result raw_archive_reader_extract_file(RawArchiveReader* self,
                                       uint32_t file_index,
                                       const char* output_path);

uint32_t raw_archive_reader_get_file_count(const RawArchiveReader* self);
const char* raw_archive_reader_get_filename(const RawArchiveReader* self,
                                            uint32_t index);

#endif  // ARCHIVE_READER_RAW_ARCHIVE_READER_H
