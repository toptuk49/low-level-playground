#include "shannon.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

typedef struct
{
  Byte symbol;
  DWord frequency;
  DWord cumulative_low;
  DWord cumulative_high;
} ShannonSymbol;

static int compare_symbols(const void* left_symbol, const void* right_symbol)
{
  const ShannonSymbol* shannon_left_symbol = (const ShannonSymbol*)left_symbol;
  const ShannonSymbol* shannon_right_symbol =
    (const ShannonSymbol*)right_symbol;

  if (shannon_left_symbol->frequency > shannon_right_symbol->frequency)
  {
    return -1;
  }

  if (shannon_left_symbol->frequency < shannon_right_symbol->frequency)
  {
    return 1;
  }

  return shannon_left_symbol->symbol < shannon_right_symbol->symbol ? -1 : 1;
}

static void generate_shannon_codes(ShannonTree* tree, ShannonSymbol* symbols,
                                   int start, int end, Byte* current_code,
                                   int depth)
{
  if (start >= end)
  {
    return;
  }

  if (start == end - 1)
  {
    Byte symbol = symbols[start].symbol;
    tree->code_lengths[symbol] = depth;

    if (depth > 0)
    {
      memcpy(tree->codes[symbol], current_code, depth);
    }

    for (int i = 0; i < SHANNON_MAX_SYMBOLS; i++)
    {
      if (tree->nodes[i] && tree->nodes[i]->symbol == symbol)
      {
        tree->nodes[i]->code_length = depth;
        memcpy(tree->nodes[i]->code, current_code, depth);
        break;
      }
    }

    return;
  }

  DWord total_frequency = 0;
  for (int i = start; i < end; i++)
  {
    total_frequency += symbols[i].frequency;
  }

  DWord half_frequency = total_frequency / 2;
  DWord current_sum = 0;
  int split_index = start;

  for (int i = start; i < end; i++)
  {
    current_sum += symbols[i].frequency;
    if (current_sum >= half_frequency)
    {
      split_index = i + 1;
      break;
    }
  }

  if (split_index <= start)
  {
    split_index = start + 1;
  }
  if (split_index >= end)
  {
    split_index = end - 1;
  }

  current_code[depth] = 0;
  generate_shannon_codes(tree, symbols, start, split_index, current_code,
                         depth + 1);

  current_code[depth] = 1;
  generate_shannon_codes(tree, symbols, split_index, end, current_code,
                         depth + 1);
}

ShannonTree* shannon_tree_create(void)
{
  ShannonTree* tree = malloc(sizeof(ShannonTree));
  if (!tree)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  tree->root = NULL;
  memset((void*)tree->nodes, 0, sizeof(tree->nodes));
  memset(tree->code_lengths, 0, sizeof(tree->code_lengths));
  memset(tree->codes, 0, sizeof(tree->codes));

  return tree;
}

static void destroy_node(ShannonNode* node)
{
  if (!node)
  {
    return;
  }

  destroy_node(node->left);
  destroy_node(node->right);
  free(node);
}

void shannon_tree_destroy(ShannonTree* tree)
{
  if (!tree)
  {
    return;
  }

  for (int i = 0; i < SHANNON_MAX_SYMBOLS; i++)
  {
    if (tree->nodes[i])
    {
      if (tree->nodes[i]->left == NULL && tree->nodes[i]->right == NULL)
      {
        free(tree->nodes[i]);
      }
      tree->nodes[i] = NULL;
    }
  }

  if (tree->root && tree->root->left == NULL && tree->root->right == NULL)
  {
    free(tree->root);
  }

  free(tree);
}

Result shannon_tree_build(ShannonTree* tree, const Byte* data, Size size)
{
  if (!tree || !data || size == 0)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  DWord frequencies[SHANNON_MAX_SYMBOLS] = {0};
  int symbol_count = 0;

  for (Size i = 0; i < size; i++)
  {
    if (frequencies[data[i]] == 0)
    {
      symbol_count++;
    }
    frequencies[data[i]]++;
  }

  ShannonSymbol* symbols =
    (ShannonSymbol*)malloc(sizeof(ShannonSymbol) * symbol_count);
  if (!symbols)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  int index = 0;
  for (int i = 0; i < SHANNON_MAX_SYMBOLS; i++)
  {
    if (frequencies[i] > 0)
    {
      symbols[index].symbol = (Byte)i;
      symbols[index].frequency = frequencies[i];
      index++;
    }
  }

  qsort(symbols, symbol_count, sizeof(ShannonSymbol), compare_symbols);

  for (int i = 0; i < symbol_count; i++)
  {
    ShannonNode* node = malloc(sizeof(ShannonNode));
    if (!node)
    {
      free(symbols);
      return RESULT_MEMORY_ERROR;
    }

    node->symbol = symbols[i].symbol;
    node->frequency = symbols[i].frequency;
    node->left = NULL;
    node->right = NULL;
    node->code_length = 0;
    memset(node->code, 0, sizeof(node->code));

    tree->nodes[symbols[i].symbol] = node;
  }

  tree->root = malloc(sizeof(ShannonNode));
  if (!tree->root)
  {
    printf("Произошла ошибка при выделении памяти!\n");

    free(symbols);
    return RESULT_MEMORY_ERROR;
  }

  tree->root->symbol = 0;
  tree->root->frequency = size;
  tree->root->left = NULL;
  tree->root->right = NULL;
  tree->root->code_length = 0;

  Byte current_code[SHANNON_MAX_CODE_LENGTH] = {0};
  generate_shannon_codes(tree, symbols, 0, symbol_count, current_code, 0);

  free(symbols);
  return RESULT_OK;
}

Size shannon_calculate_size(const ShannonTree* tree, const Byte* data,
                            Size size)
{
  if (!tree || !data)
  {
    return 0;
  }

  Size bit_count = 0;
  for (Size i = 0; i < size; i++)
  {
    bit_count += tree->code_lengths[data[i]];
  }

  Size bytes = (bit_count + 7) / 8;

  printf("[SHANNON] Расчет размера: %zu байт -> %zu бит -> %zu байт\n", size,
         bit_count, bytes);

  return bytes;
}

Result shannon_compress(const Byte* input, Size input_size, Byte** output,
                        Size* output_size, const ShannonTree* tree)
{
  if (!input || !output || !output_size || !tree || input_size == 0)
  {
    printf("[SHANNON] Ошибка: неверные параметры в shannon_compress\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[SHANNON] Начало сжатия, размер данных: %zu байт\n", input_size);

  Size compressed_size = shannon_calculate_size(tree, input, input_size);
  if (compressed_size == 0)
  {
    printf("[SHANNON] Ошибка: расчетный размер сжатия = 0\n");
    return RESULT_ERROR;
  }

  Byte* compressed = malloc(compressed_size);
  if (!compressed)
  {
    printf("[SHANNON] Ошибка выделения памяти для сжатых данных\n");
    return RESULT_MEMORY_ERROR;
  }

  memset(compressed, 0, compressed_size);
  printf("[SHANNON] Выделено %zu байт для сжатых данных\n", compressed_size);

  Size bit_position = 0;
  for (Size i = 0; i < input_size; i++)
  {
    Byte symbol = input[i];
    Byte length = tree->code_lengths[symbol];
    const Byte* code = tree->codes[symbol];

    if (length == 0)
    {
      printf("[SHANNON] ВНИМАНИЕ: символ 0x%02X не имеет кода!\n", symbol);
      free(compressed);
      return RESULT_ERROR;
    }

    for (Byte j = 0; j < length; j++)
    {
      Size byte_index = bit_position / 8;
      Size bit_index = 7 - (bit_position % 8);

      if (code[j])
      {
        compressed[byte_index] |= (1 << bit_index);
      }

      bit_position++;
    }
  }

  while (bit_position % 8 != 0)
  {
    bit_position++;  // Дополняем до границы байта
  }

  printf("[SHANNON] Сжатие завершено. Использовано бит: %zu\n", bit_position);
  printf("[SHANNON] Первые 16 байт сжатых данных: ");
  for (Size i = 0; i < 16 && i < compressed_size; i++)
  {
    printf("%02X ", compressed[i]);
  }
  printf("\n");

  *output = compressed;
  *output_size = compressed_size;
  return RESULT_OK;
}

static ShannonNode* find_symbol_node(const ShannonTree* tree, Byte symbol)
{
  if (!tree || symbol >= (Byte)SHANNON_MAX_SYMBOLS)
  {
    return NULL;
  }

  return tree->nodes[symbol];
}

Result shannon_decompress(const Byte* input, Size input_size, Byte** output,
                          Size* output_size, const ShannonTree* tree)
{
  if (!input || !output || !output_size || !tree || input_size == 0)
  {
    printf("[SHANNON] Ошибка: неверные параметры в shannon_decompress\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[SHANNON] Начало декомпрессии\n");
  printf("[SHANNON] Размер входных данных: %zu байт\n", input_size);
  printf("[SHANNON] Ожидаемый выходной размер: %zu байт\n", *output_size);

  if (input_size == 0)
  {
    printf("[SHANNON] Ошибка: входные данные пустые\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[SHANNON] Первые 16 байт сжатых данных: ");
  for (Size i = 0; i < 16 && i < input_size; i++)
  {
    printf("%02X ", input[i]);
  }
  printf("\n");

  Byte* decompressed_data = malloc(*output_size);
  if (decompressed_data == NULL)
  {
    printf(
      "[SHANNON] Ошибка выделения памяти для декомпрессированных данных\n");
    return RESULT_MEMORY_ERROR;
  }

  typedef struct
  {
    Byte code[SHANNON_MAX_CODE_LENGTH];
    Byte length;
    Byte symbol;
  } CodeTableEntry;

  CodeTableEntry code_table[SHANNON_MAX_SYMBOLS];
  int table_size = 0;

  for (int i = 0; i < SHANNON_MAX_SYMBOLS; i++)
  {
    if (tree->code_lengths[i] > 0)
    {
      code_table[table_size].symbol = (Byte)i;
      code_table[table_size].length = tree->code_lengths[i];
      memcpy(code_table[table_size].code, tree->codes[i],
             tree->code_lengths[i]);
      table_size++;
    }
  }

  Size bit_position = 0;
  Size decompressed_position = 0;

  printf("[SHANNON] Начало декодирования...\n");
  printf("[SHANNON] Всего битов для чтения: %zu\n", input_size * 8);

  while (decompressed_position < *output_size && bit_position < input_size * 8)
  {
    Byte current_code[SHANNON_MAX_CODE_LENGTH] = {0};
    Byte current_length = 0;

    while (current_length < SHANNON_MAX_CODE_LENGTH &&
           bit_position < input_size * 8)
    {
      Size byte_index = bit_position / 8;
      Size bit_index = 7 - (bit_position % 8);
      Byte bit = (input[byte_index] >> bit_index) & 1;
      bit_position++;

      current_code[current_length++] = bit;

      for (int i = 0; i < table_size; i++)
      {
        if (code_table[i].length == current_length)
        {
          bool match = true;
          for (int j = 0; j < current_length; j++)
          {
            if (current_code[j] != code_table[i].code[j])
            {
              match = false;
              break;
            }
          }

          if (match)
          {
            Byte symbol = code_table[i].symbol;
            decompressed_data[decompressed_position] = symbol;

            if (decompressed_position < 10)
            {
              printf(
                "[SHANNON] Декодирован символ %u (0x%02X '%c') на позиции %zu "
                "(бит %zu)\n",
                symbol, symbol, isprint(symbol) ? symbol : '.',
                decompressed_position, bit_position);
            }

            decompressed_position++;
            goto found_symbol;
          }
        }
      }
    }

    printf("[SHANNON] Ошибка: не найден символ для кода длиной %u\n",
           current_length);
    free(decompressed_data);
    return RESULT_ERROR;

  found_symbol:
    continue;
  }

  printf("[SHANNON] Декомпрессия завершена\n");
  printf("[SHANNON] Декомпрессировано байт: %zu из %zu ожидаемых\n",
         decompressed_position, *output_size);

  printf("[SHANNON] Первые 16 байт декомпрессированных данных: ");
  for (Size i = 0; i < 16 && i < decompressed_position; i++)
  {
    printf("%02X ", decompressed_data[i]);
  }
  printf(" (");
  for (Size i = 0; i < 16 && i < decompressed_position; i++)
  {
    printf("%c", isprint(decompressed_data[i]) ? decompressed_data[i] : '.');
  }
  printf(")\n");

  if (decompressed_position != *output_size)
  {
    printf(
      "[SHANNON] ВНИМАНИЕ: Размер не совпадает! Ожидалось: %zu, получено: "
      "%zu\n",
      *output_size, decompressed_position);

    if (decompressed_position == 0)
    {
      printf(
        "[SHANNON] КРИТИЧЕСКАЯ ОШИБКА: не декомпрессировано ни одного "
        "байта!\n");
      free(decompressed_data);
      return RESULT_ERROR;
    }

    *output_size = decompressed_position;
  }

  *output = decompressed_data;
  return RESULT_OK;
}

static void serialize_node(const ShannonNode* node, Byte* buffer,
                           Size* position, Size max_position)
{
  if (!node || *position >= max_position)
  {
    return;
  }

  if (!node->left && !node->right)
  {
    if (*position + 2 + node->code_length > max_position)
    {
      return;
    }

    buffer[(*position)++] = 1;                  // Флаг листа
    buffer[(*position)++] = node->symbol;       // Символ
    buffer[(*position)++] = node->code_length;  // Длина кода

    for (int i = 0; i < node->code_length && *position < max_position; i++)
    {
      buffer[(*position)++] = node->code[i];
    }
  }
}

Result shannon_serialize_tree(const ShannonTree* tree, Byte** data, Size* size)
{
  if (!tree || !data || !size)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  Size buffer_size = (Size)SHANNON_MAX_SYMBOLS * (3 + SHANNON_MAX_CODE_LENGTH);
  Byte* buffer = malloc(buffer_size);
  if (!buffer)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  memset(buffer, 0, buffer_size);
  Size position = 0;

  for (int i = 0; i < SHANNON_MAX_SYMBOLS; i++)
  {
    if (tree->nodes[i])
    {
      serialize_node(tree->nodes[i], buffer, &position, buffer_size);
    }
  }

  if (position == 0)
  {
    free(buffer);
    return RESULT_ERROR;
  }

  *data = malloc(position);
  if (!*data)
  {
    printf("Произошла ошибка при выделении памяти!\n");

    free(buffer);
    return RESULT_MEMORY_ERROR;
  }

  memcpy(*data, buffer, position);
  *size = position;

  free(buffer);
  return RESULT_OK;
}

static ShannonNode* deserialize_node(const Byte* buffer, Size* position,
                                     Size max_position)
{
  if (*position + 3 > max_position)
  {
    return NULL;
  }

  Byte flag = buffer[(*position)++];
  if (flag != 1)
  {
    return NULL;
  }

  Byte symbol = buffer[(*position)++];
  Byte code_length = buffer[(*position)++];

  if (*position + code_length > max_position)
  {
    return NULL;
  }

  ShannonNode* node = malloc(sizeof(ShannonNode));
  if (!node)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  node->symbol = symbol;
  node->frequency = 0;
  node->left = NULL;
  node->right = NULL;
  node->code_length = code_length;

  for (int i = 0; i < code_length; i++)
  {
    node->code[i] = buffer[(*position)++];
  }

  return node;
}

Result shannon_deserialize_tree(ShannonTree* tree, const Byte* data, Size size)
{
  if (!tree || !data || size == 0)
  {
    printf("[SHANNON] Ошибка: неверные параметры для десериализации дерева\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[SHANNON] Десериализация дерева размером %zu байт\n", size);
  printf("[SHANNON] Первые 16 байт данных дерева: ");
  for (Size i = 0; i < 16 && i < size; i++)
  {
    printf("%02X ", data[i]);
  }
  printf("\n");

  memset((void*)tree->nodes, 0, sizeof(tree->nodes));
  memset(tree->code_lengths, 0, sizeof(tree->code_lengths));
  memset(tree->codes, 0, sizeof(tree->codes));

  Size position = 0;
  int node_count = 0;

  while (position < size)
  {
    ShannonNode* node = deserialize_node(data, &position, size);
    if (!node)
    {
      break;
    }

    tree->nodes[node->symbol] = node;
    tree->code_lengths[node->symbol] = node->code_length;
    memcpy(tree->codes[node->symbol], node->code, node->code_length);
    node_count++;
  }

  printf("[SHANNON] Дерево десериализовано, узлов: %d\n", node_count);

  tree->root = malloc(sizeof(ShannonNode));
  if (!tree->root)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  tree->root->symbol = 0;
  tree->root->frequency = 0;
  tree->root->left = NULL;
  tree->root->right = NULL;
  tree->root->code_length = 0;

  return RESULT_OK;
}
