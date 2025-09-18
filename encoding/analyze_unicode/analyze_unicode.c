#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "file.h"

#define MAX_UNIQUE 65536
#define REPLACEMENT_CHAR 0xFFFDu

typedef struct {
  uint32_t codepoint;
  uint64_t count;
} Symbol;

uint32_t decodeUTF8Text(const unsigned char *currentByte, size_t bytesRemaining,
                        int *currentSymbolByteLength);
int getSymbolLength(uint32_t codepoint);
void addUniqueSymbol(uint32_t codepoint);

ProgramArguments args = {.file = NULL};
File file;
Symbol uniqueSymbols[MAX_UNIQUE];
int uniqueSymbolsCounter = 0;

int main(int argc, char **argv) {
  if (!parseArguments(&argc, &argv, &args))
    return 1;

  openFile(&file, args.file);
  readBytes(&file);

  uint64_t symbolsAmount = 0;
  size_t currentBufferPosition = 0;
  while (currentBufferPosition < (size_t)file.size) {
    int length = 0;
    uint32_t codepoint =
        decodeUTF8Text(file.buffer + currentBufferPosition,
                       file.size - currentBufferPosition, &length);
    addUniqueSymbol(codepoint);
    symbolsAmount++;
    currentBufferPosition += length;
  }

  double I_bits = 0.0;
  for (int i = 0; i < uniqueSymbolsCounter; i++) {
    double p = (double)uniqueSymbols[i].count / symbolsAmount;
    double info = -log2(p);
    I_bits += uniqueSymbols[i].count * info;
  }

  double I_bytes = I_bits / 8.0;
  uint64_t E = (uint64_t)ceil(I_bytes);

  uint64_t table_utf8 = 8; /* |A1| 64-bit */
  for (int i = 0; i < uniqueSymbolsCounter; i++)
    table_utf8 += getSymbolLength(uniqueSymbols[i].codepoint) + 8;
  uint64_t table_utf32 = 8 + uniqueSymbolsCounter * (4 + 8);
  uint64_t G_utf8 = E + table_utf8;
  uint64_t G_utf32 = E + table_utf32;

  printf("Файл: %s\n", argv[1]);
  printf("Символов Unicode: %llu\n", (unsigned long long)symbolsAmount);
  printf("I(Q)[бит]=%.2f\n", I_bits);
  printf("I(Q)[октетов]=%.2f\n", I_bytes);
  printf("E=%.0f\n", (double)E);
  printf("Размер таблицы UTF-8 +64bit freq=%llu байт\n",
         (unsigned long long)table_utf8);
  printf("Размер таблицы UTF-32 +64bit freq=%llu байт\n",
         (unsigned long long)table_utf32);
  printf("G(UTF-8)=E+table=%llu\n", (unsigned long long)G_utf8);
  printf("G(UTF-32)=E+table=%llu\n", (unsigned long long)G_utf32);

  printf("\nТаблица символов по коду:\n");
  printf("Код символа | Количество | Вероятность | Количество информации (бит) "
         "| Длина символа (байт)\n");
  for (int i = 0; i < uniqueSymbolsCounter; i++) {
    double probability = (double)uniqueSymbols[i].count / symbolsAmount;
    double info = -log2(probability);
    printf("U+%04X | %llu | %.6f | %.4f | %d\n", uniqueSymbols[i].codepoint,
           (unsigned long long)uniqueSymbols[i].count, probability, info,
           getSymbolLength(uniqueSymbols[i].codepoint));
  }

  free(file.buffer);
  fclose(file.descriptor);
  return 0;
}

uint32_t decodeUTF8Text(const unsigned char *currentByte, size_t bytesRemaining,
                        int *currentSymbolByteLength) {
  if (bytesRemaining == 0) {
    *currentSymbolByteLength = 0;
    return 0;
  }
  unsigned char c0 = currentByte[0];
  if (c0 <= 0x7F) {
    *currentSymbolByteLength = 1;
    return c0;
  }
  if ((c0 & 0xE0) == 0xC0 && bytesRemaining >= 2) {
    unsigned char c1 = currentByte[1];
    if ((c1 & 0xC0) != 0x80) {
      *currentSymbolByteLength = 1;
      return REPLACEMENT_CHAR;
    }
    *currentSymbolByteLength = 2;
    return ((c0 & 0x1F) << 6) | (c1 & 0x3F);
  }
  if ((c0 & 0xF0) == 0xE0 && bytesRemaining >= 3) {
    unsigned char c1 = currentByte[1], c2 = currentByte[2];
    if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) {
      *currentSymbolByteLength = 1;
      return REPLACEMENT_CHAR;
    }
    *currentSymbolByteLength = 3;
    return ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
  }
  if ((c0 & 0xF8) == 0xF0 && bytesRemaining >= 4) {
    unsigned char c1 = currentByte[1], c2 = currentByte[2], c3 = currentByte[3];
    if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) {
      *currentSymbolByteLength = 1;
      return REPLACEMENT_CHAR;
    }
    *currentSymbolByteLength = 4;
    return ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) |
           (c3 & 0x3F);
  }
  *currentSymbolByteLength = 1;
  return REPLACEMENT_CHAR;
}

int getSymbolLength(uint32_t codepoint) {
  if (codepoint <= 0x7F)
    return 1;
  if (codepoint <= 0x7FF)
    return 2;
  if (codepoint <= 0xFFFF)
    return 3;
  return 4;
}

void addUniqueSymbol(uint32_t codepoint) {
  for (int i = 0; i < uniqueSymbolsCounter; i++) {
    if (uniqueSymbols[i].codepoint == codepoint)
      return;
  }
  if (uniqueSymbolsCounter >= MAX_UNIQUE) {
    printf("Exceeded the number of unique symbols!\n");
    exit(1);
  }

  uniqueSymbols[uniqueSymbolsCounter].codepoint = codepoint;
  uniqueSymbols[uniqueSymbolsCounter].count = 0;
  uniqueSymbolsCounter++;
  uniqueSymbols[uniqueSymbolsCounter - 1].count++;
}
