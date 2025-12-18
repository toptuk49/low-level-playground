#include "postgres.h"

#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

struct Connection
{
  PGconn* connection;
};

static ConnectionStatus pq_status_to_connection_status(ConnStatusType pq_status)
{
  switch (pq_status)
  {
    case CONNECTION_OK:
      return CONNECTION_STATUS_OK;
    case CONNECTION_BAD:
      return CONNECTION_STATUS_BAD;
    default:
      return CONNECTION_STATUS_CONNECTING;
  }
}

static const char* pq_status_to_string(ConnStatusType pq_status)
{
  switch (pq_status)
  {
    case CONNECTION_OK:
      return "OK";
    case CONNECTION_BAD:
      return "BAD";
    case CONNECTION_STARTED:
      return "STARTED";
    case CONNECTION_MADE:
      return "MADE";
    case CONNECTION_AWAITING_RESPONSE:
      return "AWAITING_RESPONSE";
    case CONNECTION_AUTH_OK:
      return "AUTH_OK";
    case CONNECTION_SETENV:
      return "SETENV";
    case CONNECTION_SSL_STARTUP:
      return "SSL_STARTUP";
    case CONNECTION_NEEDED:
      return "NEEDED";
    default:
      return "UNKNOWN";
  }
}

Connection* connection_create(char* parameters)
{
  Connection* connection = (Connection*)malloc(sizeof(Connection));
  if (connection == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  connection->connection = PQconnectdb(parameters);
  return connection;
}

void print_connection_status(Connection* connection)
{
  ConnStatusType pq_status = PQstatus(connection->connection);
  ConnectionStatus our_status = pq_status_to_connection_status(pq_status);

  printf("Статус подключения: %s (наш: %s)\n", pq_status_to_string(pq_status),
         get_connection_status_string(our_status));

  printf("База данных: %s\n", PQdb(connection->connection));
  printf("Пользователь: %s\n", PQuser(connection->connection));
  printf("Хост: %s\n", PQhost(connection->connection));
  printf("Порт: %s\n", PQport(connection->connection));
}

ConnectionStatus get_connection_status(Connection* connection)
{
  ConnStatusType pq_status = PQstatus(connection->connection);
  return pq_status_to_connection_status(pq_status);
}

const char* get_connection_status_string(ConnectionStatus status)
{
  switch (status)
  {
    case CONNECTION_STATUS_OK:
      return "OK";
    case CONNECTION_STATUS_BAD:
      return "BAD";
    case CONNECTION_STATUS_CONNECTING:
      return "CONNECTING";
    default:
      return "UNKNOWN";
  }
}

char* get_error_message(Connection* connection)
{
  return PQerrorMessage(connection->connection);
}

void exit_with_error(Connection* connection)
{
  finish_connection(connection);
  exit(1);
}

void finish_connection(Connection* connection)
{
  if (connection && connection->connection)
  {
    PQfinish(connection->connection);
    connection->connection = NULL;
  }

  free(connection);
}

// Unsafe request
void print_query(Connection* connection, char* query)
{
  PGresult* result = PQexec(connection->connection, query);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    printf("Данные из опасного запроса не были получены! Ошибка: %s\n",
           PQerrorMessage(connection->connection));
    PQclear(result);
    return;
  }

  int rows = PQntuples(result);
  int columns = PQnfields(result);

  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < columns; j++)
    {
      printf("%s ", PQgetvalue(result, i, j));
    }
    printf("\n");
  }
  printf("\n");

  PQclear(result);
}

// Safe request
QueryParams* create_query_params(int count)
{
  QueryParams* params = (QueryParams*)malloc(sizeof(QueryParams));
  params->count = count;
  params->values = (const char**)malloc(count * sizeof(char*));
  params->lengths = (int*)malloc(count * sizeof(int));
  params->formats = (int*)malloc(count * sizeof(int));
  if (params->values == NULL || params->lengths == NULL ||
      params->formats == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  // По умолчанию все параметры в текстовом формате
  for (int i = 0; i < count; i++)
  {
    params->values[i] = NULL;
    params->lengths[i] = 0;
    params->formats[i] = 0;  // 0 = text, 1 = binary
  }

  return params;
}

void add_string_param(QueryParams* params, int index, const char* value)
{
  if (index < params->count)
  {
    size_t length = strlen(value);

    params->values[index] = (char*)malloc(length + 1);
    if (params->values[index] == NULL)
    {
      printf("Произошла ошибка при выделении памяти под параметр!\n");
      free_query_params(params);
      exit(1);
    }
    strcpy((char*)params->values[index], value);

    params->lengths[index] = (int)length;
    params->formats[index] = 0;  // Text format
  }
}

void add_int_param(QueryParams* params, int index, int value)
{
  if (index < params->count)
  {
    if (params->values[index])
    {
      free((void*)params->values[index]);
    }

    const int BUFFER_SIZE = 20;
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%d", value);
    size_t length = strlen(buffer);

    params->values[index] = (char*)malloc(length + 1);
    if (params->values[index] == NULL)
    {
      printf("Произошла ошибка при выделении памяти под строку параметра!\n");
      free_query_params(params);
      exit(1);
    }
    strcpy((char*)params->values[index], buffer);

    params->lengths[index] = (int)length;
    params->formats[index] = 0;  // Text format
  }
}

void free_query_params(QueryParams* params)
{
  if (params)
  {
    for (int i = 0; i < params->count; i++)
    {
      if (params->values[i])
      {
        free((void*)params->values[i]);
      }
    }
    free((char*)params->values);
    free(params->lengths);
    free(params->formats);
    free(params);
  }
}

// Safe request
void print_query_params(Connection* connection, char* query,
                        QueryParams* params)
{
  PGresult* result =
    PQexecParams(connection->connection, query, params->count,
                 NULL,  // OID типов (автоопределение)
                 params->values, params->lengths, params->formats,
                 0);  // результат в текстовом формате

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    printf("Данные из безопасного запроса не были получены! Error: %s\n",
           PQerrorMessage(connection->connection));
    PQclear(result);
    return;
  }

  int rows = PQntuples(result);
  int columns = PQnfields(result);

  printf("Найдено записей: %d\n", rows);
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < columns; j++)
    {
      printf("%s ", PQgetvalue(result, i, j));
    }
    printf("\n");
  }
  printf("\n");

  PQclear(result);
}

int execute_update_params(Connection* connection, char* query,
                          QueryParams* params)
{
  PGresult* result =
    PQexecParams(connection->connection, query, params->count, NULL,
                 params->values, params->lengths, params->formats, 0);

  int success = (PQresultStatus(result) == PGRES_COMMAND_OK);

  if (!success)
  {
    printf("Произошла ошибка при выполнении запроса: %s\n",
           PQerrorMessage(connection->connection));
  }
  else
  {
    printf("Запрос выполнен успешно! Затронуто строк: %s\n",
           PQcmdTuples(result));
  }

  PQclear(result);
  return success;
}
