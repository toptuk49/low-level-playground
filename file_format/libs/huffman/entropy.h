#ifndef HUFFMAN_ENTROPY_H
#define HUFFMAN_ENTROPY_H

#include "types.h"

double calculate_entropy(const Byte* data, Size size);
double calculate_information_lower_bound(const Byte* data, Size size);
double calculate_compression_ratio(Size original_size, Size compressed_size);
void analyze_file_entropy(const char* filename, Size compressed_size);

#endif  // HUFFMAN_ENTROPY_H
