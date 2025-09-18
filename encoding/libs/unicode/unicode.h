#ifndef UNICODE_H
#define UNICODE_H

#include <inttypes.h>
#include <stdlib.h>

#define MAX_UNIQUE 65536

typedef struct {
  uint32_t codepoint;
  uint64_t count;
} Symbol;

typedef struct {
  Symbol uniqueSymbols[MAX_UNIQUE];
  int uniqueSymbolsCounter;
} UnicodeStatistics;

uint32_t decodeUTF8Text(const unsigned char *currentByte, size_t bytesRemaining,
                        int *currentSymbolByteLength);
int getSymbolLength(uint32_t codepoint);
void addUniqueSymbol(UnicodeStatistics *unicodeStatistics, uint32_t codepoint);

#endif
