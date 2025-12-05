#include "huffman.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  HuffmanNode** nodes;
  Size size;
  Size capacity;
} PriorityQueue;

static PriorityQueue* priority_queue_create(Size capacity)
{
  PriorityQueue* priority_queue = malloc(sizeof(PriorityQueue));
  if (!priority_queue)
  {
    return NULL;
  }

  priority_queue->nodes =
    (HuffmanNode**)malloc(sizeof(HuffmanNode*) * capacity);

  if (!priority_queue->nodes)
  {
    free(priority_queue);
    return NULL;
  }

  priority_queue->size = 0;
  priority_queue->capacity = capacity;
  return priority_queue;
}

static void priority_queue_destroy(PriorityQueue* priority_queue)
{
  if (!priority_queue)
  {
    return;
  }

  free((void*)priority_queue->nodes);
  free(priority_queue);
}

static void priority_queue_push(PriorityQueue* priority_queue,
                                HuffmanNode* node)
{
  if (!priority_queue || !node)
  {
    return;
  }

  if (priority_queue->size >= priority_queue->capacity)
  {
    Size new_capacity = priority_queue->capacity * 2;
    HuffmanNode** new_nodes = (HuffmanNode**)realloc(
      (void*)priority_queue->nodes, sizeof(HuffmanNode*) * new_capacity);

    if (!new_nodes)
    {
      return;
    }
    priority_queue->nodes = new_nodes;
    priority_queue->capacity = new_capacity;
  }

  Size index = priority_queue->size;
  while (index > 0 &&
         priority_queue->nodes[index - 1]->frequency > node->frequency)
  {
    priority_queue->nodes[index] = priority_queue->nodes[index - 1];
    index--;
  }
  priority_queue->nodes[index] = node;
  priority_queue->size++;
}

static HuffmanNode* priority_queue_pop(PriorityQueue* priority_queue)
{
  if (!priority_queue || priority_queue->size == 0)
  {
    return NULL;
  }

  HuffmanNode* result = priority_queue->nodes[0];
  for (Size i = 1; i < priority_queue->size; i++)
  {
    priority_queue->nodes[i - 1] = priority_queue->nodes[i];
  }
  priority_queue->size--;
  return result;
}

HuffmanTree* huffman_tree_create(void)
{
  HuffmanTree* tree = malloc(sizeof(HuffmanTree));
  if (!tree)
  {
    return NULL;
  }

  tree->root = NULL;
  memset(tree->code_lengths, 0, sizeof(tree->code_lengths));
  memset(tree->codes, 0, sizeof(tree->codes));
  return tree;
}

static void destroy_node(HuffmanNode* node)
{
  if (!node)
  {
    return;
  }

  destroy_node(node->left);
  destroy_node(node->right);
  free(node);
}

void huffman_tree_destroy(HuffmanTree* tree)
{
  if (!tree)
  {
    return;
  }

  destroy_node(tree->root);
  free(tree);
}

static void generate_codes(HuffmanTree* tree, HuffmanNode* node, Byte* code,
                           int depth)
{
  if (!tree || !node)
  {
    return;
  }

  if (!node->left && !node->right)
  {
    // Лист
    tree->code_lengths[node->symbol] = depth;
    if (depth > 0)
    {
      memcpy(tree->codes[node->symbol], code, depth);
    }
    return;
  }

  if (node->left)
  {
    code[depth] = 0;
    generate_codes(tree, node->left, code, depth + 1);
  }

  if (node->right)
  {
    code[depth] = 1;
    generate_codes(tree, node->right, code, depth + 1);
  }
}

Result huffman_tree_build(HuffmanTree* tree, const Byte* data, Size size)
{
  if (!tree || !data || size == 0)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  DWord frequencies[HUFFMAN_MAX_SYMBOLS] = {0};
  for (Size i = 0; i < size; i++)
  {
    frequencies[data[i]]++;
  }

  PriorityQueue* priority_queue = priority_queue_create(HUFFMAN_MAX_SYMBOLS);
  if (!priority_queue)
  {
    return RESULT_MEMORY_ERROR;
  }

  int symbol_count = 0;
  for (int i = 0; i < HUFFMAN_MAX_SYMBOLS; i++)
  {
    if (frequencies[i] > 0)
    {
      HuffmanNode* node = malloc(sizeof(HuffmanNode));
      if (!node)
      {
        priority_queue_destroy(priority_queue);
        return RESULT_MEMORY_ERROR;
      }
      node->symbol = (Byte)i;
      node->frequency = frequencies[i];
      node->left = NULL;
      node->right = NULL;
      priority_queue_push(priority_queue, node);
      symbol_count++;
    }
  }

  if (symbol_count == 1)
  {
    HuffmanNode* node = priority_queue_pop(priority_queue);
    HuffmanNode* parent = malloc(sizeof(HuffmanNode));
    if (!parent)
    {
      free(node);
      priority_queue_destroy(priority_queue);
      return RESULT_MEMORY_ERROR;
    }
    parent->symbol = 0;
    parent->frequency = node->frequency;
    parent->left = node;
    parent->right = NULL;
    tree->root = parent;
  }
  else
  {
    while (priority_queue->size > 1)
    {
      HuffmanNode* left = priority_queue_pop(priority_queue);
      HuffmanNode* right = priority_queue_pop(priority_queue);

      HuffmanNode* parent = malloc(sizeof(HuffmanNode));
      if (!parent)
      {
        if (left)
        {
          destroy_node(left);
        }
        if (right)
        {
          destroy_node(right);
        }

        priority_queue_destroy(priority_queue);
        return RESULT_MEMORY_ERROR;
      }

      parent->symbol = 0;
      parent->frequency = left->frequency + right->frequency;
      parent->left = left;
      parent->right = right;

      priority_queue_push(priority_queue, parent);
    }

    tree->root = priority_queue_pop(priority_queue);
  }

  priority_queue_destroy(priority_queue);

  Byte code[32] = {0};
  generate_codes(tree, tree->root, code, 0);

  return RESULT_OK;
}

static void serialize_node(HuffmanNode* node, Byte* buffer, Size* position,
                           Size max_position)
{
  if (!node || *position >= max_position)
  {
    return;
  }

  if (!node->left && !node->right)
  {
    if (*position + 2 > max_position)
    {
      return;
    }

    buffer[(*position)++] = 1;
    buffer[(*position)++] = node->symbol;
  }
  else
  {
    if (*position + 1 > max_position)
    {
      return;
    }

    buffer[(*position)++] = 0;
    serialize_node(node->left, buffer, position, max_position);
    serialize_node(node->right, buffer, position, max_position);
  }
}

static HuffmanNode* deserialize_node(const Byte* buffer, Size* position,
                                     Size max_position)
{
  if (*position >= max_position)
  {
    return NULL;
  }

  Byte flag = buffer[(*position)++];

  if (flag == 1)
  {
    // Лист
    if (*position >= max_position)
    {
      return NULL;
    }

    Byte symbol = buffer[(*position)++];

    HuffmanNode* node = malloc(sizeof(HuffmanNode));
    if (!node)
    {
      return NULL;
    }

    node->symbol = symbol;
    node->frequency = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
  }
  else if (flag == 0)
  {
    // Внутренний узел
    HuffmanNode* node = malloc(sizeof(HuffmanNode));
    if (!node)
    {
      return NULL;
    }

    node->symbol = 0;
    node->frequency = 0;
    node->left = deserialize_node(buffer, position, max_position);
    node->right = deserialize_node(buffer, position, max_position);

    if (!node->left || !node->right)
    {
      destroy_node(node);
      return NULL;
    }

    return node;
  }

  return NULL;
}

Result huffman_serialize_tree(const HuffmanTree* tree, Byte** data, Size* size)
{
  if (!tree || !tree->root || !data || !size)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  // Оцениваем размер: максимально 2*256 узлов * 2 байта на узел
  Size buffer_size = HUFFMAN_MAX_SYMBOLS * 4;
  Byte* buffer = malloc(buffer_size);
  if (!buffer)
  {
    return RESULT_MEMORY_ERROR;
  }

  memset(buffer, 0, buffer_size);

  Size pos = 0;
  serialize_node(tree->root, buffer, &pos, buffer_size);

  if (pos == 0)
  {
    free(buffer);
    return RESULT_ERROR;
  }

  *data = malloc(pos);
  if (!*data)
  {
    free(buffer);
    return RESULT_MEMORY_ERROR;
  }

  memcpy(*data, buffer, pos);
  *size = pos;

  free(buffer);
  return RESULT_OK;
}

static void count_leaves(HuffmanNode* node, int* leaf_count);

Result huffman_deserialize_tree(HuffmanTree* tree, const Byte* data, Size size)
{
  if (!tree || !data || size == 0)
  {
    printf("[HUFFMAN] Ошибка: неверные параметры для десериализации дерева\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[HUFFMAN] Десериализация дерева размером %zu байт\n", size);
  printf("[HUFFMAN] Первые 16 байт данных дерева: ");
  for (Size i = 0; i < 16 && i < size; i++)
  {
    printf("%02X ", data[i]);
  }
  printf("\n");

  Size position = 0;
  tree->root = deserialize_node(data, &position, size);

  if (!tree->root)
  {
    printf("[HUFFMAN] Ошибка десериализации дерева! Позиция: %zu\n", position);
    return RESULT_ERROR;
  }

  printf("[HUFFMAN] Дерево десериализовано, позиция после чтения: %zu/%zu\n",
         position, size);

  Byte code[32] = {0};
  generate_codes(tree, tree->root, code, 0);

  int leaf_count = 0;
  count_leaves(tree->root, &leaf_count);
  printf("[HUFFMAN] Дерево содержит %d листьев\n", leaf_count);

  return RESULT_OK;
}

static void count_leaves(HuffmanNode* node, int* leaf_count)
{
  if (!node)
  {
    return;
  }

  if (!node->left && !node->right)
  {
    *leaf_count = *leaf_count + 1;
  }

  count_leaves(node->left, leaf_count);
  count_leaves(node->right, leaf_count);
}

Size huffman_calculate_size(const HuffmanTree* tree, const Byte* data,
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

  Size bytes = (bit_count + 7) / 8;  // Байты

  printf("[HUFFMAN] Расчет размера: %zu байт -> %zu бит -> %zu байт\n", size,
         bit_count, bytes);

  return bytes;
}

Result huffman_compress(const Byte* input, Size input_size, Byte** output,
                        Size* output_size, const HuffmanTree* tree)
{
  if (!input || !output || !output_size || !tree || input_size == 0)
  {
    printf("[HUFFMAN] Ошибка: неверные параметры в huffman_compress\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[HUFFMAN] Начало сжатия, размер данных: %zu байт\n", input_size);

  Size compressed_size = huffman_calculate_size(tree, input, input_size);
  if (compressed_size == 0)
  {
    printf("[HUFFMAN] Ошибка: расчетный размер сжатия = 0\n");
    return RESULT_ERROR;
  }

  Byte* compressed = malloc(compressed_size);
  if (!compressed)
  {
    printf("[HUFFMAN] Ошибка выделения памяти для сжатых данных\n");
    return RESULT_MEMORY_ERROR;
  }

  memset(compressed, 0, compressed_size);
  printf("[HUFFMAN] Выделено %zu байт для сжатых данных\n", compressed_size);

  Size bit_position = 0;
  for (Size i = 0; i < input_size; i++)
  {
    Byte symbol = input[i];
    Byte length = tree->code_lengths[symbol];
    const Byte* code = tree->codes[symbol];

    if (length == 0)
    {
      printf("[HUFFMAN] ВНИМАНИЕ: символ 0x%02X не имеет кода!\n", symbol);
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
    Size byte_index = bit_position / 8;
    Size bit_index = 7 - (bit_position % 8);
    compressed[byte_index] &= ~(1 << bit_index);  // Устанавливаем 0
    bit_position++;
  }

  printf("[HUFFMAN] Сжатие завершено. Использовано бит: %zu\n", bit_position);
  printf("[HUFFMAN] Первые 16 байт сжатых данных: ");
  for (Size i = 0; i < 16 && i < compressed_size; i++)
  {
    printf("%02X ", compressed[i]);
  }
  printf("\n");

  *output = compressed;
  *output_size = compressed_size;
  return RESULT_OK;
}

Result huffman_decompress(const Byte* input, Size input_size, Byte** output,
                          Size* output_size, const HuffmanTree* tree)
{
  if (!input || !output || !output_size || !tree || !tree->root)
  {
    printf("[HUFFMAN] Ошибка: неверные параметры в huffman_decompress\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[HUFFMAN] Начало декомпрессии\n");
  printf("[HUFFMAN] Размер входных данных: %zu байт\n", input_size);
  printf("[HUFFMAN] Ожидаемый выходной размер: %zu байт\n", *output_size);

  if (input_size == 0)
  {
    printf("[HUFFMAN] Ошибка: входные данные пустые\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[HUFFMAN] Первые 16 байт сжатых данных: ");
  for (Size i = 0; i < 16 && i < input_size; i++)
  {
    printf("%02X ", input[i]);
  }
  printf("\n");

  Size bit_position = 0;
  Size decompressed_position = 0;
  const HuffmanNode* current_node = tree->root;
  Byte* decompressed_data = malloc(*output_size);

  if (decompressed_data == NULL)
  {
    printf(
      "[HUFFMAN] Ошибка выделения памяти для декомпрессированных данных\n");
    return RESULT_MEMORY_ERROR;
  }

  printf("[HUFFMAN] Начало декодирования...\n");
  printf("[HUFFMAN] Всего битов для чтения: %zu\n", input_size * 8);

  if (!tree->root->left && !tree->root->right)
  {
    printf("[HUFFMAN] ВНИМАНИЕ: корень дерева является листом!\n");
    while (decompressed_position < *output_size &&
           bit_position < input_size * 8)
    {
      Size byte_index = bit_position / 8;
      bit_position++;

      decompressed_data[decompressed_position++] = tree->root->symbol;
    }
  }
  else
  {
    while (decompressed_position < *output_size &&
           bit_position < input_size * 8)
    {
      Size byte_index = bit_position / 8;
      Size bit_index = 7 - (bit_position % 8);
      int bit = (input[byte_index] >> bit_index) & 1;
      bit_position++;

      if (bit_position <= 32)
      {
        printf("[HUFFMAN] Битов %zu: %d\n", bit_position, bit);
      }

      if (bit == 0)
      {
        current_node = current_node->left;
      }
      else
      {
        current_node = current_node->right;
      }

      if (!current_node)
      {
        printf("[HUFFMAN] Ошибка: достигнут NULL узел в дереве на бите %zu\n",
               bit_position);
        printf("[HUFFMAN] Уже декомпрессировано: %zu байт\n",
               decompressed_position);
        free(decompressed_data);
        return RESULT_ERROR;
      }

      if (!current_node->left && !current_node->right)
      {
        Byte symbol = current_node->symbol;
        decompressed_data[decompressed_position] = symbol;

        if (decompressed_position < 10)
        {
          printf(
            "[HUFFMAN] Декодирован символ %u (0x%02X '%c') на позиции %zu (бит "
            "%zu)\n",
            symbol, symbol, isprint(symbol) ? symbol : '.',
            decompressed_position, bit_position);
        }

        decompressed_position++;
        current_node = tree->root;
      }
    }
  }

  printf("[HUFFMAN] Декомпрессия завершена\n");
  printf("[HUFFMAN] Декомпрессировано байт: %zu из %zu ожидаемых\n",
         decompressed_position, *output_size);

  printf("[HUFFMAN] Первые 16 байт декомпрессированных данных: ");
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
      "[HUFFMAN] ВНИМАНИЕ: Размер не совпадает! Ожидалось: %zu, получено: "
      "%zu\n",
      *output_size, decompressed_position);

    if (decompressed_position == 0)
    {
      printf(
        "[HUFFMAN] КРИТИЧЕСКАЯ ОШИБКА: не декомпрессировано ни одного "
        "байта!\n");
      free(decompressed_data);
      return RESULT_ERROR;
    }

    *output_size = decompressed_position;
  }

  *output = decompressed_data;
  return RESULT_OK;
}
