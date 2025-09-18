#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "file.h"
#include "unicode.h"

UnicodeStatistics initUnicodeStatistics();

File file;
ProgramArguments args = {.file = NULL};
UnicodeStatistics unicodeStatistics;
uint64_t symbolsAmount = 0;

int main(int argc, char **argv) {
  if (!parseArguments(&argc, &argv, &args))
    return 1;

  openFile(&file, args.file);
  readBytes(&file);

  unicodeStatistics = initUnicodeStatistics();

  size_t currentBufferPosition = 0;
  while (currentBufferPosition < (size_t)file.size) {
    int length = 0;
    uint32_t codepoint =
        decodeUTF8Text(file.buffer + currentBufferPosition,
                       file.size - currentBufferPosition, &length);
    addUniqueSymbol(&unicodeStatistics, codepoint);
    symbolsAmount++;
    currentBufferPosition += length;
  }

  double I_bits = 0.0;
  for (int i = 0; i < unicodeStatistics.uniqueSymbolsCounter; i++) {
    double p = (double)unicodeStatistics.uniqueSymbols[i].count / symbolsAmount;
    double info = -log2(p);
    I_bits += unicodeStatistics.uniqueSymbols[i].count * info;
  }

  double I_bytes = I_bits / 8.0;
  uint64_t E = (uint64_t)ceil(I_bytes);

  uint64_t table_utf8 = 8; /* |A1| 64-bit */
  for (int i = 0; i < unicodeStatistics.uniqueSymbolsCounter; i++)
    table_utf8 +=
        getSymbolLength(unicodeStatistics.uniqueSymbols[i].codepoint) + 8;
  uint64_t table_utf32 = 8 + unicodeStatistics.uniqueSymbolsCounter * (4 + 8);
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
  for (int i = 0; i < unicodeStatistics.uniqueSymbolsCounter; i++) {
    double probability =
        (double)unicodeStatistics.uniqueSymbols[i].count / symbolsAmount;
    double info = -log2(probability);
    printf("U+%04X | %llu | %.6f | %.4f | %d\n",
           unicodeStatistics.uniqueSymbols[i].codepoint,
           (unsigned long long)unicodeStatistics.uniqueSymbols[i].count,
           probability, info,
           getSymbolLength(unicodeStatistics.uniqueSymbols[i].codepoint));
  }

  free(file.buffer);
  fclose(file.descriptor);
  return 0;
}

UnicodeStatistics initUnicodeStatistics() {
  UnicodeStatistics s = {.uniqueSymbolsCounter = 0};
  return s;
}
