#ifndef ARCHIVE_BUILDER_COMPRESSED_ARCHIVE_BUILDER_H
#define ARCHIVE_BUILDER_COMPRESSED_ARCHIVE_BUILDER_H

#include "types.h"

typedef struct CompressedArchiveBuilder CompressedArchiveBuilder;

CompressedArchiveBuilder* compressed_archive_builder_create(
  const char* output_filename);
void compressed_archive_builder_destroy(CompressedArchiveBuilder* self);

Result compressed_archive_builder_add_file(CompressedArchiveBuilder* self,
                                           const char* filename);
Result compressed_archive_builder_add_directory(CompressedArchiveBuilder* self,
                                                const char* dirname);

Result compressed_archive_builder_finalize(CompressedArchiveBuilder* self);

#endif  // ARCHIVE_BUILDER_COMPRESSED_ARCHIVE_BUILDER_H
