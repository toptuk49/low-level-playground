#ifndef COMPRESSED_ARCHIVE_CODEC_CODER_H
#define COMPRESSED_ARCHIVE_CODEC_CODER_H

#include "types.h"

Result compressed_archive_encode(const char* input_path,
                                 const char* output_filename);
Result compressed_archive_encode_extended(const char* input_path,
                                          const char* output_filename,
                                          const char* algorithm);

#endif  // COMPRESSED_ARCHIVE_CODEC_CODER_H
