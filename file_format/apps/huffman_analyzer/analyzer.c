#include "analyzer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entropy.h"
#include "file.h"
#include "huffman.h"
#include "types.h"

#define TABLE_SIZE 256

static DWord* normalize_frequencies(const DWord* original_frequencies,
                                    Size data_size, BitDepth depth,
                                    Size* normalized_size)
{
  if (original_frequencies == NULL || data_size == 0)
  {
    return NULL;
  }

  DWord max_original = 0;
  for (int i = 0; i < TABLE_SIZE; i++)
  {
    if (original_frequencies[i] > max_original)
    {
      max_original = original_frequencies[i];
    }
  }

  if (max_original == 0)
  {
    return NULL;
  }

  DWord max_target = (1ULL << depth) - 1;
  DWord* normalized = malloc(TABLE_SIZE * sizeof(DWord));

  if (normalized == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  for (int i = 0; i < TABLE_SIZE; i++)
  {
    if (original_frequencies[i] == 0)
    {
      normalized[i] = 0;
    }
    else
    {
      double scale = (double)max_target / max_original;
      DWord value = (DWord)ceil(original_frequencies[i] * scale);
      if (value == 0)
      {
        value = 1;  // Ненулевые частоты не должны стать нулевыми
      }
      normalized[i] = value;
    }
  }

  const Byte byte = 8;
  *normalized_size = (Size)TABLE_SIZE * (depth / byte);
  return normalized;
}

static HuffmanTree* build_tree_from_normalized_frequencies(
  const DWord* normalized_frequencies)
{
  if (normalized_frequencies == NULL)
  {
    return NULL;
  }

  Size total_symbols = 0;
  for (int i = 0; i < TABLE_SIZE; i++)
  {
    total_symbols += normalized_frequencies[i];
  }

  if (total_symbols == 0)
  {
    return NULL;
  }

  Byte* synthetic_data = (Byte*)malloc(total_symbols * sizeof(Byte));
  if (synthetic_data == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  Size position = 0;
  for (int i = 0; i < TABLE_SIZE; i++)
  {
    for (DWord j = 0; j < normalized_frequencies[i]; j++)
    {
      synthetic_data[position++] = (Byte)i;
    }
  }

  HuffmanTree* tree = huffman_tree_create();
  if (tree == NULL)
  {
    free(synthetic_data);
    return NULL;
  }

  Result result = huffman_tree_build(tree, synthetic_data, total_symbols);
  free(synthetic_data);

  if (result != RESULT_OK)
  {
    huffman_tree_destroy(tree);
    return NULL;
  }

  return tree;
}

CompressionResult analyze_file_bit_depth(const char* filename, BitDepth depth)
{
  CompressionResult result = {0, 0};

  if (filename == NULL || depth == 0)
  {
    return result;
  }

  File* file = file_create(filename);

  if (file == NULL)
  {
    return result;
  }

  if (file_open_for_read(file) != RESULT_OK)
  {
    file_destroy(file);
    return result;
  }

  if (file_read_bytes(file) != RESULT_OK)
  {
    file_close(file);
    file_destroy(file);
    return result;
  }

  const Byte* data = file_get_buffer(file);
  Size size = file_get_size(file);

  if (size == 0)
  {
    file_close(file);
    file_destroy(file);
    return result;
  }

  DWord original_frequencies[TABLE_SIZE] = {0};
  for (Size i = 0; i < size; i++)
  {
    original_frequencies[data[i]]++;
  }

  Size frequency_size = 0;
  DWord* normalized_frequencies =
    normalize_frequencies(original_frequencies, size, depth, &frequency_size);

  if (normalized_frequencies == NULL)
  {
    file_close(file);
    file_destroy(file);
    return result;
  }

  HuffmanTree* tree =
    build_tree_from_normalized_frequencies(normalized_frequencies);
  free(normalized_frequencies);

  if (tree == NULL)
  {
    file_close(file);
    file_destroy(file);
    return result;
  }

  result.E = huffman_calculate_size(tree, data, size);

  const Byte byte = 8;
  Size freq_bytes = (Size)TABLE_SIZE * (depth / byte);

  result.G = result.E + freq_bytes;

  huffman_tree_destroy(tree);
  file_close(file);
  file_destroy(file);

  return result;
}

void analyze_file_all_bit_depths(const char* filename)
{
  if (filename == NULL)
  {
    return;
  }

  printf("\n========================================\n");
  printf("Анализ файла: %s\n", filename);
  printf("========================================\n");

  File* file = file_create(filename);
  if (file == NULL)
  {
    return;
  }

  if (file_open_for_read(file) != RESULT_OK)
  {
    file_destroy(file);
    return;
  }

  if (file_read_bytes(file) != RESULT_OK)
  {
    file_close(file);
    file_destroy(file);
    return;
  }

  const Byte* data = file_get_buffer(file);
  Size size = file_get_size(file);

  printf("Размер файла: %zu байт\n\n", size);

  double entropy = calculate_entropy(data, size);
  double info_bits = calculate_information_lower_bound(data, size);
  const double byte = 8.0;
  double info_bytes = info_bits / byte;

  printf("Энтропийный анализ:\n");
  printf("  Энтропия: %.4f бит/символ\n", entropy);
  printf("  Оценка снизу H: %.2f бит (%.2f байт)\n", info_bits, info_bytes);
  printf("  Теоретический минимум сжатия: %.2f%%\n",
         (1.0 - info_bytes / (double)size) * 100);

  BitDepth depths[] = {64, 32, 16, 8, 4, 2, 1};
  const char* depth_names[] = {"64-бит", "32-бит", "16-бит", "8-бит",
                               "4-бит",  "2-бит",  "1-бит"};

  CompressionResult best_result = {SIZE_MAX, SIZE_MAX};
  BitDepth best_depth = 64;

  printf("\nАнализ для разных разрядностей частот:\n");
  printf("┌───────────┬────────────┬────────────┬────────────┬────────────┐\n");
  printf(
    "│ Разрядность │ E (байт)   │ Частоты    │ G (байт)   │ Эффективность │\n");
  printf("├───────────┼────────────┼────────────┼────────────┼────────────┤\n");

  for (int i = 0; i < sizeof(depths) / sizeof(depths[0]); i++)
  {
    CompressionResult result = analyze_file_bit_depth(filename, depths[i]);

    if (result.E > 0)
    {
      double efficiency = (1.0 - (double)result.G / size) * 100;

      const Byte byte = 8;
      printf("│ %-9s │ %-10zu │ %-10u │ %-10zu │ %-10.2f%% │\n", depth_names[i],
             result.E, TABLE_SIZE * (depths[i] / byte), result.G, efficiency);

      if (result.G < best_result.G)
      {
        best_result = result;
        best_depth = depths[i];
      }
    }
  }

  printf("└───────────┴────────────┴────────────┴────────────┴────────────┘\n");

  printf("\nОптимальная разрядность B*: %d-бит\n", best_depth);
  printf("Минимальный общий размер G_min: %zu байт\n", best_result.G);
  printf("Экономия с оптимальной разрядностью: %.2f%%\n",
         (1.0 - (double)best_result.G / (double)size) * 100);

  printf("\nРекомендация по фиксированной разрядности B**:\n");
  printf("  8-бит: удобно для чтения/записи, минимальные накладные расходы\n");
  printf("  Представление 256 частот: 256 × 1 байт = 256 байт\n");

  file_close(file);
  file_destroy(file);
}

BitDepth find_optimal_bit_depth(const char* filename)
{
  const Byte byte = 8;

  if (filename == NULL)
  {
    return byte;
  }

  BitDepth depths[] = {64, 32, 16, 8, 4, 2, 1};
  BitDepth best_depth = byte;
  Size min_G = SIZE_MAX;

  for (int i = 0; i < sizeof(depths) / sizeof(depths[0]); i++)
  {
    CompressionResult result = analyze_file_bit_depth(filename, depths[i]);
    if (result.G > 0 && result.G < min_G)
    {
      min_G = result.G;
      best_depth = depths[i];
    }
  }

  return best_depth;
}

void create_plots_for_files(const char** filenames, int count)
{
  if (filenames == NULL || count == 0)
  {
    return;
  }

  printf("\nГенерация данных для графиков...\n");
  printf(
    "Файл,Размер,Энтропия,E64,E32,E16,E8,E4,E2,E1,G64,G32,G16,G8,G4,G2,G1\n");

  for (int i = 0; i < count; i++)
  {
    File* file = file_create(filenames[i]);
    if (file == NULL)
    {
      continue;
    }

    if (file_open_for_read(file) != RESULT_OK)
    {
      file_destroy(file);
      continue;
    }

    if (file_read_bytes(file) != RESULT_OK)
    {
      file_close(file);
      file_destroy(file);
      continue;
    }

    const Byte* data = file_get_buffer(file);
    Size size = file_get_size(file);
    double entropy = calculate_entropy(data, size);

    printf("%s,%zu,%.4f", filenames[i], size, entropy);

    BitDepth depths[] = {64, 32, 16, 8, 4, 2, 1};
    for (int depth = 0; depth < sizeof(depths) / sizeof(depths[0]); depth++)
    {
      CompressionResult result =
        analyze_file_bit_depth(filenames[i], depths[depth]);
      printf(",%zu", result.E);
    }

    for (int depth = 0; depth < sizeof(depths) / sizeof(depths[0]); depth++)
    {
      CompressionResult result =
        analyze_file_bit_depth(filenames[i], depths[depth]);
      printf(",%zu", result.G);
    }

    printf("\n");

    file_close(file);
    file_destroy(file);
  }

  printf("\nДля построения графиков вставьте следующий код в Jupyter Lab:\n");
  printf("import pandas as pd\n");
  printf("import matplotlib.pyplot as plt\n");
  printf("df = pd.read_csv('huffman_analysis.csv')\n");
  printf("plt.subplots()\n");
  printf("for i in range(len(df)):\n");
  printf(
    "    plt.plot([64,32,16,8,4,2,1], df.iloc[i,3:10], marker='o', "
    "label=df.iloc[i,0])\n");
  printf("plt.xlabel('Разрядность B (бит)')\n");
  printf("plt.ylabel('Длина сжатых данных E_B (байт)')\n");
  printf("plt.title('Зависимость E_B от B для разных файлов')\n");
  printf("plt.legend()\n");
  printf("plt.grid(True)\n");
  printf("plt.savefig('E_vs_B.png')\n");
  printf("plt.show()\n");
  printf("\"\"\n");
}
