#ifndef ARCHIVE_BUILDER_RAW_ARCHIVE_BUILDER_H
#define ARCHIVE_BUILDER_RAW_ARCHIVE_BUILDER_H

#include "types.h"

typedef struct RawArchiveBuilder RawArchiveBuilder;

RawArchiveBuilder* raw_archive_builder_create(const char* output_filename);
void raw_archive_builder_destroy(RawArchiveBuilder* self);

Result raw_archive_builder_add_file(RawArchiveBuilder* self,
                                    const char* filename);
Result raw_archive_builder_add_directory(RawArchiveBuilder* self,
                                         const char* dirname);

Result raw_archive_builder_finalize(RawArchiveBuilder* self);

#endif  // ARCHIVE_BUILDER_RAW_ARCHIVE_BUILDER_H
