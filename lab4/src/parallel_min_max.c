#define _POSIX_SOURCE
#include <getopt.h>
#include <limits.h>
#include <signal.h>
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
#define DESCRIPTORS_COUNT 2
#define READ_DESCRIPTOR 0
#define PIPE_ERROR -1
#define WRITE_DESCRIPTOR 1
#define CHILD_PROCESS 0
#define PROCESS_ERROR -1
#define WAIT_ANY_CHILD -1
#define WAIT_CHILD_BY_PID 0
#define WAIT_CHILD_GROUP 0
#define WAIT_NO_CHILD 0
#define WAIT_ANY_CHILD_BY_PID(pid) pid
#define NORMAL 0
#define STILL_ALIVE 0
#define NO_OPTION -1
#define BASE 10

typedef struct
{
  unsigned long long seed;
  unsigned long long array_size;
  int process_count;
  bool use_files;
  int timeout;
} ProgramArguments;

bool ParseArguments(int argc, char **argv, ProgramArguments *arguments);
void RunChildProcess(int process_index, int *array,
                     unsigned long long array_size, int process_count,
                     bool use_files, int pipe_fd[2]);
void SaveMinMaxToFile(int process_index, struct MinMax result);
void LoadMinMaxFromFile(int process_index, struct MinMax *result);
void TimeoutHandler(int signal);
struct MinMax CollectResultsFromChildren(int process_count, bool use_files,
                                         int pipes[][2]);

volatile sig_atomic_t timeout_flag = 0;

// 1. Считать и проверить аргументы
// 2. Сгенерировать массив
// 3. Засечь время
// 4. Запустить дочерние процессы
// 5. Дождаться всех дочерних процессов
// 6. Если таймаут сработал - завершить дочерние процессы и программу
// 7. Собрать min/max от дочерних процессов
// 8. Вывести результат

int main(int argc, char **argv)
{
  ProgramArguments args = {.seed = -1,
                           .array_size = -1,
                           .process_count = -1,
                           .use_files = false,
                           .timeout = -1};

  if (!ParseArguments(argc, argv, &args))
  {
    return 1;
  }

  signal(SIGALRM, TimeoutHandler);

  int *array = malloc(sizeof(int) * args.array_size);
  if (array == NULL)
  {
    perror("Not enough memory for specified size");
  }

  GenerateArray(array, args.array_size, args.seed);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int pipes[args.process_count][DESCRIPTORS_COUNT];
  for (int process_index = 0; process_index < args.process_count;
       process_index++)
  {
    if (!args.use_files && pipe(pipes[process_index]) == PIPE_ERROR)
    {
      perror("Pipe failed");
      return 1;
    }

    pid_t child_pid = fork();
    if (child_pid == CHILD_PROCESS)
    {
      RunChildProcess(process_index, array, args.array_size, args.process_count,
                      args.use_files, pipes[process_index]);
      return 0;
    }

    if (child_pid == PROCESS_ERROR)
    {
      perror("Fork failed");
      return 1;
    }
  }

  if (args.timeout > 0)
  {
    alarm(args.timeout);
  }

  for (int i = 0; i < args.process_count; i++)
  {
    pid_t pid = waitpid(WAIT_ANY_CHILD, NULL, NORMAL);

    if (pid == PROCESS_ERROR)
    {
      perror("Wait failed");
      return 1;
    }
  }

  if (timeout_flag)
  {
    for (int process_index = 0; process_index < args.process_count;
         process_index++)
    {
      if (kill(pipes[process_index][READ_DESCRIPTOR], CHILD_PROCESS) ==
          STILL_ALIVE)
      {
        kill(pipes[process_index][READ_DESCRIPTOR], SIGKILL);
      }
    }

    printf("Timeout occured. Exiting the program.\n");
    return 1;
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
    {"timeout", required_argument, 0, 0},
    {0, 0, 0, 0}};

  arguments->timeout = 0;
  arguments->use_files = false;

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
            arguments->seed = strtoull(optarg, NULL, BASE);
            break;
          case 1:
            arguments->array_size = strtoull(optarg, NULL, BASE);
            break;
          case 2:
            arguments->process_count = atoi(optarg);
            break;
          case 3:
            arguments->use_files = true;
            break;
          case 4:
            arguments->timeout = atoi(optarg);
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
      arguments->process_count <= 0 || arguments->timeout < 0)
  {
    printf(
      "Usage: --seed <num> --array_size <num> --pnum <num> [--by_files] "
      "[--timeout <num>]\n");
    return false;
  }

  return true;
}

void RunChildProcess(int process_index, int *array,
                     unsigned long long array_size, int process_count,
                     bool use_files, int pipe_fd[DESCRIPTORS_COUNT])
{
  unsigned long long segment_size = array_size / process_count;
  unsigned long long start_index = process_index * segment_size;
  unsigned long long end_index = (process_index == process_count - 1)
                                   ? array_size
                                   : start_index + segment_size;

  struct MinMax result = GetMinMax(array, start_index, end_index);

  if (use_files)
  {
    SaveMinMaxToFile(process_index, result);
  }
  else
  {
    close(pipe_fd[READ_DESCRIPTOR]);
    write(pipe_fd[WRITE_DESCRIPTOR], &result, sizeof(result));
    close(pipe_fd[WRITE_DESCRIPTOR]);
  }

  exit(0);
}

void SaveMinMaxToFile(int process_index, struct MinMax result)
{
  char filename[256];
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
  char filename[256];
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

void TimeoutHandler(int signal)
{
  if (signal == SIGALRM)
  {
    printf("Timeout reached. Sending SIGKILL to all running child processes");
  }

  timeout_flag = 1;
}

struct MinMax CollectResultsFromChildren(int process_count, bool use_files,
                                         int pipes[][2])
{
  struct MinMax global_result;
  global_result.min = INT_MAX;
  global_result.max = INT_MIN;

  for (int process_index = 0; process_index < process_count; process_index++)
  {
    struct MinMax local_result;

    if (use_files)
    {
      LoadMinMaxFromFile(process_index, &local_result);
    }
    else
    {
      close(pipes[process_index][WRITE_DESCRIPTOR]);
      read(pipes[process_index][READ_DESCRIPTOR], &local_result,
           sizeof(local_result));
      close(pipes[process_index][READ_DESCRIPTOR]);
    }

    if (local_result.min < global_result.min)
      global_result.min = local_result.min;
    if (local_result.max > global_result.max)
      global_result.max = local_result.max;
  }

  return global_result;
}
