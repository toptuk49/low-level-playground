#include <inttypes.h>
#include <stdio.h>

#include "unicode.h"

#define REPLACEMENT_CHAR 0xFFFDu

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

void addUniqueSymbol(UnicodeStatistics *unicodeStatistics, uint32_t codepoint) {
  for (int i = 0; i < unicodeStatistics->uniqueSymbolsCounter; i++) {
    if (unicodeStatistics->uniqueSymbols[i].codepoint == codepoint)
      return;
  }
  if (unicodeStatistics->uniqueSymbolsCounter >= MAX_UNIQUE) {
    printf("Exceeded the number of unique symbols!\n");
    exit(1);
  }

  unicodeStatistics->uniqueSymbols[unicodeStatistics->uniqueSymbolsCounter]
      .codepoint = codepoint;
  unicodeStatistics->uniqueSymbols[unicodeStatistics->uniqueSymbolsCounter]
      .count = 0;
  unicodeStatistics->uniqueSymbolsCounter++;
  unicodeStatistics->uniqueSymbols[unicodeStatistics->uniqueSymbolsCounter - 1]
      .count++;
}
