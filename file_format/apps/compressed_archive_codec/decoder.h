#ifndef COMPRESSED_ARCHIVE_CODEC_DECODER_H
#define COMPRESSED_ARCHIVE_CODEC_DECODER_H

#include "types.h"

Result compressed_archive_decode(const char* input_filename,
                                 const char* output_path);

#endif  // COMPRESSED_ARCHIVE_CODEC_DECODER_H
