#ifndef HUFFMAN_HUFFMAN_H
#define HUFFMAN_HUFFMAN_H

#include "types.h"

#define HUFFMAN_MAX_SYMBOLS 256

typedef struct HuffmanNode
{
  Byte symbol;
  DWord frequency;
  struct HuffmanNode* left;
  struct HuffmanNode* right;
} HuffmanNode;

typedef struct
{
  HuffmanNode* root;
  Byte codes[HUFFMAN_MAX_SYMBOLS][32];  // Коды длиной до 32 бит
  Byte code_lengths[HUFFMAN_MAX_SYMBOLS];
} HuffmanTree;

// Основные функции
HuffmanTree* huffman_tree_create(void);
void huffman_tree_destroy(HuffmanTree* tree);
Result huffman_tree_build(HuffmanTree* tree, const Byte* data, Size size);

Result huffman_compress(const Byte* input, Size input_size, Byte** output,
                        Size* output_size, const HuffmanTree* tree);

Result huffman_decompress(const Byte* input, Size input_size, Byte** output,
                          Size* output_size, const HuffmanTree* tree);

// Сериализация дерева
Result huffman_serialize_tree(const HuffmanTree* tree, Byte** data, Size* size);
Result huffman_deserialize_tree(HuffmanTree* tree, const Byte* data, Size size);

// Утилиты
Size huffman_calculate_size(const HuffmanTree* tree, const Byte* data,
                            Size size);

#endif  // HUFFMAN_HUFFMAN_H
