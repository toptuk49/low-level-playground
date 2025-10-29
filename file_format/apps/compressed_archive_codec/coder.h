#ifndef COMPRESSED_ARCHIVE_CODEC_CODER_H
#define COMPRESSED_ARCHIVE_CODEC_CODER_H

#include "types.h"

Result compressed_archive_encode(const char* input_path,
                                 const char* output_filename);

#endif  // COMPRESSED_ARCHIVE_CODEC_CODER_H
