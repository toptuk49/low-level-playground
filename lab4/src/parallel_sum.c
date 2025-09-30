#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

#include "../libs/sum.h"
#include "../libs/utils.h"

#define true 1
#define false 0
#define BASE 10

typedef struct
{
  unsigned long long seed;
  unsigned long long array_size;
  int threads_num;
} ProgramArguments;

bool ParseArguments(int argc, char **argv, ProgramArguments *arguments);
void *ThreadSum(void *args);

int main(int argc, char **argv)
{
  ProgramArguments programArguments = {
    .seed = -1, .array_size = -1, .threads_num = -1};

  if (!ParseArguments(argc, argv, &programArguments))
    return 1;

  pthread_t threads[programArguments.threads_num];

  int *array = malloc(sizeof(int) * programArguments.array_size);
  if (array == NULL)
  {
    printf("Not enough memory for array of size %llu\n",
           programArguments.array_size);
    return 1;
  }

  GenerateArray(array, programArguments.array_size, programArguments.seed);

  struct SumArgs args[programArguments.threads_num];

  int chunk_size = programArguments.array_size / programArguments.threads_num;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < programArguments.threads_num; i++)
  {
    args[i].array = array;
    args[i].begin = i * chunk_size;
    args[i].end = (i == programArguments.threads_num - 1)
                    ? programArguments.array_size
                    : (i + 1) * chunk_size;

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args))
    {
      printf("Error: pthread_create failed!\n");
      free(array);
      return 1;
    }
  }

  int total_sum = 0;
  for (int i = 0; i < programArguments.threads_num; i++)
  {
    void *sum_ptr;
    pthread_join(threads[i], (void **)&sum_ptr);
    total_sum += (int)(size_t)sum_ptr;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  printf("Total: %d\n", total_sum);
  printf("Elapsed time: %.2f ms\n", elapsed_time);

  free(array);
  return 0;
}

bool ParseArguments(int argc, char **argv, ProgramArguments *arguments)
{
  int option_index = 0;
  static struct option long_options[] = {
    {"seed", required_argument, 0, 0},
    {"array_size", required_argument, 0, 0},
    {"threads_num", required_argument, 0, 0},
    {0, 0, 0, 0}};

  while (true)
  {
    int current_option =
      getopt_long(argc, argv, "", long_options, &option_index);
    if (current_option == -1)
      break;

    switch (current_option)
    {
      case 0:
        switch (option_index)
        {
          case 0:
            arguments->seed = strtoull(optarg, NULL, BASE);
            break;
          case 1:
            arguments->array_size = strtoull(optarg, NULL, BASE);
            break;
          case 2:
            arguments->threads_num = atoi(optarg);
            break;
        }
        break;
      default:
        printf("Invalid arguments\n");
        return false;
    }
  }

  if (arguments->seed <= 0 || arguments->array_size <= 0 ||
      arguments->threads_num <= 0)
  {
    printf("Usage: --seed <num> --array_size <num> --threads_num <num>\n");
    return false;
  }

  return true;
}

void *ThreadSum(void *args)
{
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}
