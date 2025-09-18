#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdio.h>

#define ALPHABET_SIZE 256

typedef struct {
  long byteCounts[ALPHABET_SIZE];
  long byteLength;
  double byteAppearProbabilites[ALPHABET_SIZE];
  double byteInformationAmounts[ALPHABET_SIZE];
} Statistics;

typedef unsigned char byte;

Statistics countStatistics(FILE *file);

#endif
