#ifndef SHANNON_SHANNON_H
#define SHANNON_SHANNON_H

#include "types.h"

#define SHANNON_MAX_SYMBOLS 256
#define SHANNON_MAX_CODE_LENGTH 32

typedef struct ShannonNode
{
  Byte symbol;
  DWord frequency;
  struct ShannonNode* left;
  struct ShannonNode* right;
  Byte code[SHANNON_MAX_CODE_LENGTH];
  Byte code_length;
} ShannonNode;

typedef struct
{
  ShannonNode* root;
  ShannonNode* nodes[SHANNON_MAX_SYMBOLS];
  Byte codes[SHANNON_MAX_SYMBOLS][SHANNON_MAX_CODE_LENGTH];
  Byte code_lengths[SHANNON_MAX_SYMBOLS];
} ShannonTree;

ShannonTree* shannon_tree_create(void);
void shannon_tree_destroy(ShannonTree* tree);
Result shannon_tree_build(ShannonTree* tree, const Byte* data, Size size);

Result shannon_compress(const Byte* input, Size input_size, Byte** output,
                        Size* output_size, const ShannonTree* tree);

Result shannon_decompress(const Byte* input, Size input_size, Byte** output,
                          Size* output_size, const ShannonTree* tree);

Result shannon_serialize_tree(const ShannonTree* tree, Byte** data, Size* size);
Result shannon_deserialize_tree(ShannonTree* tree, const Byte* data, Size size);

Size shannon_calculate_size(const ShannonTree* tree, const Byte* data,
                            Size size);

#endif  // SHANNON_SHANNON_H
