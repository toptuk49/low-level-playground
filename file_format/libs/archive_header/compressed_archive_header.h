#ifndef ARCHIVE_HEADER_COMPRESSED_ARCHIVE_HEADER_H
#define ARCHIVE_HEADER_COMPRESSED_ARCHIVE_HEADER_H

#include <stdbool.h>
#include <stdint.h>

#include "file.h"
#include "types.h"

#define COMPRESSED_ARCHIVE_SIGNATURE "lolkek"
#define COMPRESSED_ARCHIVE_SIGNATURE_SIZE 6
#define COMPRESSED_ARCHIVE_VERSION_MAJOR 1
#define COMPRESSED_ARCHIVE_VERSION_MINOR 0

typedef enum
{
  COMPRESSION_NONE = 0,
  COMPRESSION_HUFFMAN = 1,
  // И так далее для разных алгоритмов
} CompressionAlgorithm;

typedef enum
{
  ERROR_CORRECTION_NONE = 0,
  ERROR_CORRECTION_CRC32 = 1,
} ErrorCorrectionAlgorithm;

typedef enum
{
  FLAG_NONE = 0,
  FLAG_DIRECTORY = 1 << 0,   // Архив содержит папки
  FLAG_COMPRESSED = 1 << 1,  // Данные сжаты
  FLAG_ENCRYPTED = 1 << 2,   // Данные зашифрованы
  FLAG_METADATA = 1 << 3     // Содержит метаданные
} CompressedArchiveFlags;

typedef struct
{
  // Базовый заголовок
  char signature[COMPRESSED_ARCHIVE_SIGNATURE_SIZE];
  uint16_t version_major;
  uint16_t version_minor;
  uint64_t original_size;

  // Алгоритмы
  uint8_t primary_compression;
  uint8_t secondary_compression;
  uint8_t error_correction;

  // Флаги и служебные данные
  uint32_t flags;
  uint32_t header_size;      // Полный размер заголовка
  uint32_t metadata_size;    // Размер метаданных
  uint32_t compressed_size;  // Размер сжатых данных

  // Контрольные суммы
  uint32_t header_crc;
  uint32_t data_crc;
} CompressedArchiveHeader;

#define COMPRESSED_ARCHIVE_HEADER_SIZE (sizeof(CompressedArchiveHeader))

bool compressed_archive_header_is_valid(const CompressedArchiveHeader* header);
Result compressed_archive_header_init(CompressedArchiveHeader* header,
                                      uint64_t original_size,
                                      uint8_t primary_compression,
                                      uint8_t secondary_compression,
                                      uint8_t error_correction, uint32_t flags);
Result compressed_archive_header_write(const CompressedArchiveHeader* header,
                                       File* file);
Result compressed_archive_header_read(CompressedArchiveHeader* header,
                                      File* file);

#endif  // ARCHIVE_HEADER_COMPRESSED_ARCHIVE_HEADER_H
