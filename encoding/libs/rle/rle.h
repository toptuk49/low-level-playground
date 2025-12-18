#ifndef RLE_RLE_H
#define RLE_RLE_H

#include "types.h"

typedef struct RLEContext
{
  Byte prefix;
} RLEContext;

RLEContext* rle_create(Byte prefix);
void rle_destroy(RLEContext* context);

Result rle_compress(const Byte* input, Size input_size, Byte** output,
                    Size* output_size, const RLEContext* context);
Result rle_decompress(const Byte* input, Size input_size, Byte** output,
                      Size* output_size, const RLEContext* context);

Result rle_serialize_context(const RLEContext* context, Byte** data,
                             Size* size);
Result rle_deserialize_context(RLEContext* context, const Byte* data,
                               Size size);

Byte rle_get_prefix(const RLEContext* context);
Result rle_set_prefix(RLEContext* context, Byte prefix);
Byte rle_analyze_prefix(const Byte* data, Size size);
void rle_test_compression(const Byte* data, Size size, Byte prefix);

#endif  // RLE_RLE_H
