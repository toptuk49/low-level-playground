#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../libs/find_min_max.h"
#include "../libs/utils.h"

#define FILENAME_TEMPLATE "child_result_%d.txt"
#define BASE 10

typedef struct
{
  int seed;
  int array_size;
  int process_count;
  bool use_files;
} ProgramArguments;

bool ParseArguments(int argc, char **argv, ProgramArguments *arguments);
void RunChildProcess(int process_index, int *array, int array_size,
                     int process_count, bool use_files, int pipe_fd[2]);
void SaveMinMaxToFile(int process_index, struct MinMax result);
void LoadMinMaxFromFile(int process_index, struct MinMax *result);
struct MinMax CollectResultsFromChildren(int process_count, bool use_files,
                                         int pipes[][2]);

// 1. Считать и проверить аргументы
// 2. Сгенерировать массив
// 3. Засечь время
// 4. Запустить дочерние процессы
// 5. Дождаться всех дочерних процессов
// 6. Собрать min/max от дочерних процессов
// 7. Вывести результат

int main(int argc, char **argv)
{
  ProgramArguments args = {
    .seed = -1, .array_size = -1, .process_count = -1, .use_files = false};

  if (!ParseArguments(argc, argv, &args))
  {
    return 1;
  }

  int *array = malloc(sizeof(int) * args.array_size);
  GenerateArray(array, args.array_size, args.seed);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int pipes[args.process_count][2];
  for (int process_index = 0; process_index < args.process_count;
       process_index++)
  {
    if (!args.use_files && pipe(pipes[process_index]) == -1)
    {
      perror("pipe");
      return 1;
    }

    pid_t child_pid = fork();
    if (child_pid == 0)
    {
      RunChildProcess(process_index, array, args.array_size, args.process_count,
                      args.use_files, pipes[process_index]);
    }
    else if (child_pid < 0)
    {
      perror("fork");
      return 1;
    }
  }

  // Ждем завершения всех дочерних процессов
  for (int i = 0; i < args.process_count; i++)
  {
    wait(NULL);
  }

  struct MinMax global_result =
    CollectResultsFromChildren(args.process_count, args.use_files, pipes);

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  const double second = 1000.0;
  double elapsed_ms = (double)(finish_time.tv_sec - start_time.tv_sec) * second;
  elapsed_ms += (double)(finish_time.tv_usec - start_time.tv_usec) / second;

  printf("Min: %d\n", global_result.min);
  printf("Max: %d\n", global_result.max);
  printf("Elapsed time: %.2f ms\n", elapsed_ms);

  free(array);
  return 0;
}

bool ParseArguments(int argc, char **argv, ProgramArguments *arguments)
{
  int option_index = 0;
  static struct option long_options[] = {
    {"seed", required_argument, 0, 0},
    {"array_size", required_argument, 0, 0},
    {"pnum", required_argument, 0, 0},
    {"by_files", no_argument, 0, 'f'},
    {0, 0, 0, 0}};

  while (true)
  {
    int current_option =
      getopt_long(argc, argv, "f", long_options, &option_index);
    if (current_option == -1)
    {
      break;
    }

    switch (current_option)
    {
      case 0:
        switch (option_index)
        {
          case 0:
            arguments->seed = (int)strtol(optarg, NULL, BASE);
            break;
          case 1:
            arguments->array_size = (int)strtol(optarg, NULL, BASE);
            break;
          case 2:
            arguments->process_count = (int)strtol(optarg, NULL, BASE);
            break;
          case 3:
            arguments->use_files = true;
            break;
        }
        break;
      case 'f':
        arguments->use_files = true;
        break;
      default:
        printf("Invalid arguments\n");
        return false;
    }
  }

  if (arguments->seed <= 0 || arguments->array_size <= 0 ||
      arguments->process_count <= 0)
  {
    printf(
      "Usage: --seed <num> --array_size <num> --pnum <num> [--by_files]\n");
    return false;
  }

  return true;
}

void RunChildProcess(int process_index, int *array, int array_size,
                     int process_count, bool use_files, int pipe_fd[2])
{
  int segment_size = array_size / process_count;
  int start_index = process_index * segment_size;
  int end_index = (process_index == process_count - 1)
                    ? array_size
                    : start_index + segment_size;

  struct MinMax result = GetMinMax(array, start_index, end_index);

  if (use_files)
  {
    SaveMinMaxToFile(process_index, result);
  }
  else
  {
    close(pipe_fd[0]);  // Закрываем чтение
    write(pipe_fd[1], &result, sizeof(result));
    close(pipe_fd[1]);
  }

  exit(0);  // Завершаем дочерний процесс
}

void SaveMinMaxToFile(int process_index, struct MinMax result)
{
  char filename[UCHAR_MAX];
  snprintf(filename, sizeof(filename), FILENAME_TEMPLATE, process_index);

  FILE *file = fopen(filename, "w");
  if (!file)
  {
    perror("fopen");
    exit(1);
  }

  fprintf(file, "%d %d\n", result.min, result.max);
  fclose(file);
}

void LoadMinMaxFromFile(int process_index, struct MinMax *result)
{
  char filename[UCHAR_MAX];
  snprintf(filename, sizeof(filename), FILENAME_TEMPLATE, process_index);

  FILE *file = fopen(filename, "r");
  if (!file)
  {
    perror("fopen");
    exit(1);
  }

  fscanf(file, "%d %d", &result->min, &result->max);
  fclose(file);
  remove(filename);
}

struct MinMax CollectResultsFromChildren(int process_count, bool use_files,
                                         int pipes[][2])
{
  struct MinMax global_result;
  global_result.min = INT_MAX;
  global_result.max = INT_MIN;

  for (int i = 0; i < process_count; i++)
  {
    struct MinMax local_result;

    if (use_files)
    {
      LoadMinMaxFromFile(i, &local_result);
    }
    else
    {
      close(pipes[i][1]);  // Закрываем запись
      read(pipes[i][0], &local_result, sizeof(local_result));
      close(pipes[i][0]);
    }

    if (local_result.min < global_result.min)
    {
      global_result.min = local_result.min;
    }
    if (local_result.max > global_result.max)
    {
      global_result.max = local_result.max;
    }
  }

  return global_result;
}
