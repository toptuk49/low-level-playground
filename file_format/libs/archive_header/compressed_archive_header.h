#ifndef ARCHIVE_HEADER_COMPRESSED_ARCHIVE_HEADER_H
#define ARCHIVE_HEADER_COMPRESSED_ARCHIVE_HEADER_H

#include <stdbool.h>

#include "file.h"
#include "types.h"

#define COMPRESSED_ARCHIVE_SIGNATURE "lolkek"
#define COMPRESSED_ARCHIVE_SIGNATURE_SIZE 6
#define COMPRESSED_ARCHIVE_VERSION_MAJOR 2
#define COMPRESSED_ARCHIVE_VERSION_MINOR \
  1  // Увеличили для поддержки двухэтапного сжатия

typedef enum
{
  COMPRESSION_NONE = 0,
  COMPRESSION_HUFFMAN = 1,
  COMPRESSION_ARITHMETIC = 2,
  COMPRESSION_SHANNON = 3,
  COMPRESSION_RLE = 4,
  COMPRESSION_LZ78 = 5,
  COMPRESSION_LZ77 = 6,
} CompressionAlgorithm;

typedef enum
{
  ERROR_CORRECTION_NONE = 0,
  ERROR_CORRECTION_CRC32 = 1,
} ErrorCorrectionAlgorithm;

typedef enum
{
  FLAG_NONE = 0,
  FLAG_DIRECTORY = 1 << 0,               // Архив содержит папки
  FLAG_COMPRESSED = 1 << 1,              // Данные сжаты
  FLAG_ENCRYPTED = 1 << 2,               // Данные зашифрованы
  FLAG_METADATA = 1 << 3,                // Содержит метаданные
  FLAG_HUFFMAN_TREE = 1 << 4,            // Содержит дерево Хаффмана
  FLAG_ARITHMETIC_MODEL = 1 << 5,        // Содержит арифметическую модель
  FLAG_SHANNON_TREE = 1 << 6,            // Содержит дерево Шеннона-Фано
  FLAG_RLE_CONTEXT = 1 << 7,             // Содержит контекст RLE
  FLAG_LZ78_CONTEXT = 1 << 8,            // Содержит контекст LZ78
  FLAG_LZ77_CONTEXT = 1 << 9,            // Содержит контекст LZ77
  FLAG_TWO_STAGE_COMPRESSION = 1 << 10,  // Используется двухэтапное сжатие
} CompressedArchiveFlags;

typedef struct
{
  // Базовый заголовок
  char signature[COMPRESSED_ARCHIVE_SIGNATURE_SIZE];
  Word version_major;
  Word version_minor;
  QWord original_size;

  // Алгоритмы
  Byte primary_compression;
  Byte secondary_compression;  // Для двухэтапного сжатия
  Byte error_correction;

  // Флаги и служебные данные
  DWord flags;
  DWord header_size;      // Полный размер заголовка
  DWord metadata_size;    // Размер метаданных
  DWord compressed_size;  // Размер сжатых данных

  // Размеры моделей/деревьев для первичного алгоритма
  DWord
    primary_tree_model_size;  // Общий размер дерева/модели первичного алгоритма
  DWord huffman_tree_size;    // Размер сериализованного дерева Хаффмана
  DWord arithmetic_model_size;  // Размер сериализованной арифметической модели
  DWord shannon_tree_size;      // Размер сериализованного дерева Шеннона-Фано

  // Размеры контекстов для вторичного алгоритма
  DWord secondary_context_size;  // Общий размер контекста вторичного алгоритма
  DWord rle_context_size;        // Размер контекста RLE
  DWord lz78_context_size;       // Размер контекста LZ78
  DWord lz77_context_size;       // Размер контекста LZ77

  // Контрольные суммы
  DWord header_crc;
  DWord data_crc;
} CompressedArchiveHeader;

#define COMPRESSED_ARCHIVE_HEADER_SIZE (sizeof(CompressedArchiveHeader))

bool compressed_archive_header_is_valid(const CompressedArchiveHeader* header);
Result compressed_archive_header_init(CompressedArchiveHeader* header,
                                      QWord original_size,
                                      Byte primary_compression,
                                      Byte secondary_compression,
                                      Byte error_correction, DWord flags);
Result compressed_archive_header_write(const CompressedArchiveHeader* header,
                                       File* file);
Result compressed_archive_header_read(CompressedArchiveHeader* header,
                                      File* file);

#endif  // ARCHIVE_HEADER_COMPRESSED_ARCHIVE_HEADER_H
