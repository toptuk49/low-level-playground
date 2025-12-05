#ifndef HUFFMAN_ANALYZER_ANALYZER_H
#define HUFFMAN_ANALYZER_ANALYZER_H

#include "types.h"

typedef struct
{
  Size E;  // Длина сжатых данных (байты)
  Size G;  // Общая длина (E + размер частот) (байты)
} CompressionResult;

typedef enum
{
  BIT_DEPTH_64 = 64,
  BIT_DEPTH_32 = 32,
  BIT_DEPTH_16 = 16,
  BIT_DEPTH_8 = 8,
  BIT_DEPTH_4 = 4,
  BIT_DEPTH_2 = 2,
  BIT_DEPTH_1 = 1
} BitDepth;

void analyze_file_all_bit_depths(const char* filename);

CompressionResult analyze_file_bit_depth(const char* filename, BitDepth depth);

BitDepth find_optimal_bit_depth(const char* filename);

void create_plots_for_files(const char** filenames, int count);

#endif  // HUFFMAN_ANALYZER_ANALYZER_H
