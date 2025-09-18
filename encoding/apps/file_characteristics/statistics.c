#include <math.h>
#include <stdlib.h>

#include "statistics.h"

Statistics initStatistics();
void countProbabilities(Statistics *statistics);
void countInformationAmounts(Statistics *statistics);

Statistics countStatistics(FILE *file) {
  Statistics statistics = initStatistics();

  byte b;
  while (fread(&b, 1, 1, file) == 1) {
    statistics.byteCounts[b]++;
    statistics.byteLength++;
  }

  if (statistics.byteLength == 0) {
    printf("Файл пустой!\n");
    exit(0);
  }

  countProbabilities(&statistics);
  countInformationAmounts(&statistics);

  return statistics;
}

Statistics initStatistics() {
  Statistics s = {.byteCounts = {0}, .byteLength = 0};

  return s;
}

void countProbabilities(Statistics *statistics) {
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    statistics->byteAppearProbabilites[i] =
        (statistics->byteCounts[i] > 0)
            ? (double)statistics->byteCounts[i] / statistics->byteLength
            : 0.0;
  }
}

void countInformationAmounts(Statistics *statistics) {
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    statistics->byteInformationAmounts[i] =
        (statistics->byteAppearProbabilites[i] > 0)
            ? -log2(statistics->byteAppearProbabilites[i])
            : 0.0;
  }
}
