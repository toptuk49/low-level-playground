#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "arguments.h"
#include "file.h"
#include "statistics.h"
#include "summary.h"

#define ALPHABET_SIZE 256

int compareByteCounts(const void *leftBytePointer,
                      const void *rightBytePointer);

ProgramArguments args = {.file = NULL};
File file;
Statistics statistics;
Summary summary;

int main(int argc, char **argv) {
  if (!parseArguments(&argc, &argv, &args))
    return 1;

  openFile(&file, args.file);

  statistics = countStatistics(file.descriptor);
  summary = countSummary(statistics);

  printf("Файл: %s\n", argv[1]);
  printf("Длина файла n = %ld байт\n", statistics.byteLength);
  printf("L(Q)[бит] = %ld\n", statistics.byteLength * 8);
  printf("I(Q)[бит] = %.2f\n", summary.informationBits);
  printf("Дробная часть I(Q)[бит] = %.2e\n",
         fmod(summary.informationBits, 1.0));
  printf("L(Q)[октетов] = %ld\n", statistics.byteLength);
  printf("I(Q)[октетов] = %.2f\n", summary.informationBytes);
  printf("E (нижняя граница) = %ld\n", summary.compressedTextMinSize);
  printf("G64 = %ld\n", summary.archiveLengthWithNonStandardFrequenciesTable);
  printf("G8 = %ldn\n", summary.archiveLengthWithNormalizedFrequenciesTable);

  printf("Таблица (по алфавиту):\n");
  printf("Байт | Количество | Вероятность байта | Информация в байте\n");
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    printf("%02X | %9ld | %11.6f | %8.4f\n", i, statistics.byteCounts[i],
           statistics.byteAppearProbabilites[i],
           statistics.byteInformationAmounts[i]);
  }

  printf("\nТаблица (по убыванию количества символов):\n");
  printf("Байт | Количество | Вероятность байта | Информация в байте\n");
  int indices[ALPHABET_SIZE];
  for (int i = 0; i < ALPHABET_SIZE; i++)
    indices[i] = i;
  qsort(indices, ALPHABET_SIZE, sizeof(int), compareByteCounts);

  for (int k = 0; k < ALPHABET_SIZE; k++) {
    int i = indices[k];
    printf("%02X   | %9ld | %11.6f | %8.4f\n", i, statistics.byteCounts[i],
           statistics.byteAppearProbabilites[i],
           statistics.byteInformationAmounts[i]);
  }

  fclose(file.descriptor);
  return 0;
}

int compareByteCounts(const void *leftBytePointer,
                      const void *rightBytePointer) {
  int leftByte = *(const int *)leftBytePointer;
  int rightByte = *(const int *)rightBytePointer;

  if (statistics.byteCounts[rightByte] > statistics.byteCounts[leftByte])
    return 1;
  if (statistics.byteCounts[rightByte] < statistics.byteCounts[leftByte])
    return -1;

  return 0;
}
