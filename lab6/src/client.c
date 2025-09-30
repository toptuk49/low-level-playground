#include <arpa/inet.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "../libs/client_arguments.h"
#include "../libs/safe_modulo_multiplication.h"

#define PORT_LIMIT 65536
#define BASE 10

struct Server
{
  char ip[UCHAR_MAX];
  int port;
};

struct ThreadData
{
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  bool success;
};

void* ServerThread(void* arg);
int ReadServersFromFile(const char* filename, struct Server** servers);

ProgramArguments args = {.k = -1, .mod = -1};

int main(int argc, char** argv)
{
  memset(&args.servers, 0, sizeof(args.servers));

  if (!parseArguments(&argc, &argv, &args))
  {
    return 1;
  }

  FILE* file_descriptor = fopen(&args.servers, "r");
  if (file_descriptor == NULL)
  {
    printf("Произошла ошибка при открытии файла!\n");
    return 1;
  }

  int move_status = fseek(file_descriptor, 0, SEEK_END);
  if (move_status != 0)
  {
    printf("Произошла ошибка при перемещении в конец файла!\n");
    return 1;
  }
  uint64_t file_size_in_bytes = ftell(file_descriptor);
  move_status = fseek(file_descriptor, 0, SEEK_SET);
  if (move_status != 0)
  {
    printf("Произошла ошибка при перемещении в начало файла!\n");
    return 1;
  }

  struct Server* servers = NULL;
  int servers_count = ReadServersFromFile(&args.servers, &servers);
  if (servers_count <= 0)
  {
    printf("Не удалось загрузить серверы из %s!\n", &args.servers);
    free(servers);
    return 1;
  }

  printf("Загружено %d серверов\n", servers_count);
  printf("Вычисляем %zu! mod %zu\n", args.k, args.mod);

  pthread_t* threads = malloc(sizeof(pthread_t) * servers_count);
  struct ThreadData* thread_data =
    malloc(sizeof(struct ThreadData) * servers_count);

  uint64_t numbers_per_server = args.k / servers_count;
  uint64_t remainder = args.k % servers_count;
  uint64_t current_start = 1;

  for (int i = 0; i < servers_count; i++)
  {
    thread_data[i].server = servers[i];
    thread_data[i].mod = args.mod;
    thread_data[i].begin = current_start;

    thread_data[i].end = current_start + numbers_per_server - 1;
    if (i < remainder)
    {
      thread_data[i].end++;
    }

    current_start = thread_data[i].end + 1;

    printf("Сервер %s:%d: диапазон %lu-%lu\n", servers[i].ip, servers[i].port,
           thread_data[i].begin, thread_data[i].end);

    if (pthread_create(&threads[i], NULL, ServerThread, &thread_data[i]) != 0)
    {
      printf("Ошибка создания потока для сервера %d\n", i);
      thread_data[i].success = false;
    }
  }

  for (int i = 0; i < servers_count; i++)
  {
    pthread_join(threads[i], NULL);
  }

  uint64_t final_result = 1;
  int successful_servers = 0;

  for (int i = 0; i < servers_count; i++)
  {
    if (thread_data[i].success)
    {
      final_result =
        SafeModuloMultiplication(final_result, thread_data[i].result, args.mod);
      successful_servers++;
    }
    else
    {
      printf("Сервер %s:%d не ответил\n", servers[i].ip, servers[i].port);
    }
  }

  if (successful_servers == servers_count)
  {
    printf("\nУспех! %lu! mod %lu = %lu\n", args.k, args.mod, final_result);
  }
  else if (successful_servers > 0)
  {
    printf("\nЧастичный успех (%d/%d серверов): %lu! mod %lu = %lu\n",
           successful_servers, servers_count, args.k, args.mod, final_result);
  }
  else
  {
    printf("\nВсе серверы не ответили\n");
    final_result = 0;
  }

  free(servers);

  return 0;
}

void* ServerThread(void* arg)
{
  struct ThreadData* data = (struct ThreadData*)arg;

  struct hostent* hostname = gethostbyname(data->server.ip);
  if (hostname == NULL)
  {
    printf("Не удалось получить хост по имени %s!\n", data->server.ip);
    data->success = false;
    return NULL;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(data->server.port);
  server_addr.sin_addr.s_addr = *((unsigned long*)hostname->h_addr_list[0]);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    printf("Не удалось создать сокет для %s:%d!\n", data->server.ip,
           data->server.port);
    data->success = false;
    return NULL;
  }

  struct timeval timeout;
  const int seconds = 5;
  timeout.tv_sec = seconds;
  timeout.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Не удалось подключиться к %s:%d!\n", data->server.ip,
           data->server.port);
    close(sock);
    data->success = false;
    return NULL;
  }

  uint64_t task[3];
  task[0] = data->begin;
  task[1] = data->end;
  task[2] = data->mod;

  if (send(sock, task, sizeof(task), 0) < 0)
  {
    printf("Не удалось отправить данные на %s:%d!\n", data->server.ip,
           data->server.port);
    close(sock);
    data->success = false;
    return NULL;
  }

  uint64_t response;
  ssize_t bytes_received = recv(sock, &response, sizeof(response), MSG_WAITALL);
  if (bytes_received != sizeof(uint64_t))
  {
    printf("Неполный ответ от %s:%d! Получено %zd байт\n", data->server.ip,
           data->server.port, bytes_received);
    close(sock);
    data->success = false;
    return NULL;
  }

  data->result = response;
  data->success = true;

  printf("Сервер %s:%d: факториал [%lu-%lu] mod %lu = %lu\n", data->server.ip,
         data->server.port, data->begin, data->end, data->mod, data->result);

  close(sock);
  return NULL;
}

int ReadServersFromFile(const char* filename, struct Server** servers)
{
  FILE* file = fopen(filename, "r");
  if (file == NULL)
  {
    printf("Не удалось открыть файл %s!\n", filename);
    return -1;
  }

  const int capacity_start = 10;
  int capacity = capacity_start;
  int count = 0;
  *servers = malloc(sizeof(struct Server) * capacity);

  char line[UCHAR_MAX];
  while (fgets(line, sizeof(line), file))
  {
    line[strcspn(line, "\n")] = '\0';

    if (strlen(line) == 0)
    {
      continue;
    }

    char* colon = strchr(line, ':');
    if (colon == NULL)
    {
      printf("Некорректный формат: %s (ожидается ip:port)\n", line);
      continue;
    }

    *colon = '\0';
    char* ip_address = line;
    int port = (int)strtol(colon + 1, NULL, BASE);

    if (port <= 0 || port > PORT_LIMIT)
    {
      printf("Некорректный порт: %d\n", port);
      continue;
    }

    if (count >= capacity)
    {
      capacity *= 2;
      *servers = realloc(*servers, sizeof(struct Server) * capacity);
    }

    strncpy((*servers)[count].ip, ip_address, sizeof((*servers)[count].ip) - 1);
    (*servers)[count].ip[sizeof((*servers)[count].ip) - 1] = '\0';
    (*servers)[count].port = port;
    count++;
  }

  int close_status = fclose(file);
  if (close_status != 0)
  {
    printf("Произошла ошибка при закрытии файла!\n");
    return 1;
  }

  return count;
}
