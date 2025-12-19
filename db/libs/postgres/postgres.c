#include "postgres.h"

#include <ctype.h>
#include <libpq-fe.h>
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"

#define INT4OID 23
#define TEXTOID 25

struct Connection
{
  PGconn* connection;
  LogLevel log_level;
  bool in_transaction;
};

struct PreparedStatement
{
  char* name;
  char* query;
  int n_params;
};

static LogLevel global_log_level = LOG_INFO;

static const char* sql_injection_patterns[] = {
  "('(''|[^'])*')",
  "(\\b(SELECT|UPDATE|INSERT|DELETE|DROP|ALTER|CREATE|TRUNCATE)\\b)",
  "(\\b(OR|AND)\\s+['\"]?\\d+['\"]?\\s*[=<>]\\s*['\"]?\\d+['\"]?)",
  "(;\\s*\\w)",
  "(--|/\\*|\\*/)",
  "(UNION\\s+ALL\\s+SELECT)",
  "(EXEC\\s*\\()",
  "(WAITFOR\\s+DELAY)",
  "(xp_cmdshell)",
  "(CHAR\\()",
  NULL};

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

// ==================== АУТЕНТИФИКАЦИЯ И СЕССИИ ====================

UserSession* create_user_session()
{
  UserSession* session = (UserSession*)malloc(sizeof(UserSession));
  if (session == NULL)
  {
    postgres_log(LOG_ERROR, "Ошибка выделения памяти для сессии");
    return NULL;
  }

  session->username = NULL;
  session->role = USER_ROLE_UNKNOWN;
  session->student_id = -1;
  session->is_authenticated = 0;

  return session;
}

void destroy_user_session(UserSession* session)
{
  if (session)
  {
    if (session->username)
    {
      free(session->username);
    }
    free(session);
    postgres_log(LOG_DEBUG, "Сессия пользователя уничтожена");
  }
}

int authenticate_user(Connection* conn, UserSession* session,
                      const char* username, const char* password)
{
  if (conn == NULL || session == NULL || username == NULL || password == NULL)
  {
    postgres_log(LOG_ERROR, "Некорректные параметры для аутентификации");
    return 0;
  }

  if (conn->connection)
  {
    PQfinish(conn->connection);
  }

  const int BUFFER_SIZE = 512;
  char connection_params[BUFFER_SIZE];
  snprintf(
    connection_params, sizeof(connection_params),
    "host=localhost port=5432 user=%s password=%s dbname=enhanced_students",
    username, password);

  conn->connection = PQconnectdb(connection_params);

  if (PQstatus(conn->connection) != CONNECTION_OK)
  {
    postgres_log(LOG_ERROR, "Аутентификация не удалась: %s",
                 PQerrorMessage(conn->connection));
    return 0;
  }

  session->role = detect_user_role(conn, username);
  session->is_authenticated = 1;

  if (session->username)
  {
    free(session->username);
  }
  session->username = strdup(username);

  if (session->role == USER_ROLE_USER || session->role == USER_ROLE_JUNIOR)
  {
    char* query = "SELECT student_id FROM students WHERE last_name = $1";
    QueryParams* params = create_string_param(username);

    PGresult* result =
      PQexecParams(conn->connection, query, 1, NULL, params->values,
                   params->lengths, params->formats, 0);

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0)
    {
      const int BASE = 10;
      session->student_id = (int)strtol(PQgetvalue(result, 0, 0), NULL, BASE);
    }

    PQclear(result);
    free_query_params(params);
  }

  postgres_log(LOG_INFO, "Пользователь %s аутентифицирован как %s", username,
               get_user_role_string(session->role));
  return 1;
}

UserRole detect_user_role(Connection* conn, const char* username)
{
  if (conn == NULL || username == NULL)
  {
    return USER_ROLE_UNKNOWN;
  }

  const int BUFFER_SIZE = 256;
  char check_admin_query[BUFFER_SIZE];
  snprintf(check_admin_query, sizeof(check_admin_query),
           "SELECT rolsuper FROM pg_roles WHERE rolname = '%s'", username);

  PGresult* result = PQexec(conn->connection, check_admin_query);

  if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0)
  {
    char* is_super = PQgetvalue(result, 0, 0);
    if (strcmp(is_super, "t") == 0)
    {
      PQclear(result);
      return USER_ROLE_ADMIN;
    }
  }

  PQclear(result);

  if (strstr(username, "admin") != NULL || strstr(username, "Admin") != NULL)
  {
    return USER_ROLE_ADMIN;
  }

  if (strstr(username, "junior") != NULL || strstr(username, "Junior") != NULL)
  {
    return USER_ROLE_JUNIOR;
  }

  return USER_ROLE_USER;
}

const char* get_user_role_string(UserRole role)
{
  switch (role)
  {
    case USER_ROLE_ADMIN:
      return "Администратор";
    case USER_ROLE_JUNIOR:
      return "Младший пользователь (только чтение)";
    case USER_ROLE_USER:
      return "Обычный пользователь";
    default:
      return "Неизвестная роль";
  }
}

int has_permission(UserSession* session, QueryType query_type,
                   const char* table)
{
  if (session == NULL || !session->is_authenticated)
  {
    postgres_log(LOG_WARNING, "Попытка доступа без аутентификации");
    return 0;
  }

  if (session->role == USER_ROLE_ADMIN)
  {
    return 1;
  }

  if (session->role == USER_ROLE_JUNIOR)
  {
    if (query_type == QUERY_SELECT)
    {
      return 1;
    }
    postgres_log(LOG_WARNING,
                 "Пользователь %s пытается выполнить операцию %d без прав",
                 session->username, query_type);
    return 0;
  }

  if (session->role == USER_ROLE_USER)
  {
    if (query_type == QUERY_SELECT)
    {
      return 1;
    }

    if ((query_type == QUERY_UPDATE || query_type == QUERY_DELETE) &&
        table != NULL)
    {
      if (strcmp(table, "field_comprehensions") == 0)
      {
        return 1;
      }
    }
  }

  return 0;
}

// ==================== ЛОГИРОВАНИЕ ====================

void set_log_level(LogLevel level)
{
  global_log_level = level;
}

void postgres_log(LogLevel level, const char* format, ...)
{
  if (level < global_log_level)
  {
    return;
  }

  const char* level_str;
  switch (level)
  {
    case LOG_DEBUG:
      level_str = "DEBUG";
      break;
    case LOG_INFO:
      level_str = "INFO";
      break;
    case LOG_WARNING:
      level_str = "WARNING";
      break;
    case LOG_ERROR:
      level_str = "ERROR";
      break;
    case LOG_SECURITY:
      level_str = "SECURITY";
      break;
    default:
      level_str = "UNKNOWN";
      break;
  }

  printf("[%s] ", level_str);

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  printf("\n");
}

void log_sql_injection_attempt(const char* query, const char* user_input)
{
  postgres_log(
    LOG_SECURITY,
    "Обнаружена попытка SQL-инъекции! Запрос: %s, Ввод пользователя: %s", query,
    user_input);
}

void log_operation(UserSession* session, const char* operation,
                   const char* details)
{
  if (session == NULL || operation == NULL)
  {
    return;
  }

  time_t now = time(NULL);
  const char BUFFER_SIZE = 64;
  char timestamp[BUFFER_SIZE];
  size_t bytes_written = strftime(timestamp, sizeof(timestamp),
                                  "%Y-%m-%d %H:%M:%S", localtime(&now));

  postgres_log(LOG_INFO, "[%s] %s: %s (Пользователь: %s, Роль: %s)", timestamp,
               operation, details,
               session->username ? session->username : "unknown",
               get_user_role_string(session->role));
}

// ==================== МЕТОДЫ ЗАЩИТЫ ОТ SQL-ИНЪЕКЦИЙ ====================

void correct_apostrof(char* query)
{
  for (size_t i = 0; i < strlen(query); ++i)
  {
    if (query[i] == '\'')
    {
      query[i] = '`';
    }
  }

  postgres_log(LOG_DEBUG, "Кавычки заменены на апострофы");
}

void add_escape_quotes(const char* query, char* query_new, size_t buffer_size)
{
  size_t right_pointer = 0;
  for (size_t left_pointer = 0;
       query[left_pointer] != '\0' && right_pointer < buffer_size - 1;
       ++left_pointer)
  {
    if (query[left_pointer] == '\'')
    {
      if (right_pointer + 2 < buffer_size - 1)
      {
        query_new[right_pointer] = '\'';
        query_new[right_pointer + 1] = '\'';
        right_pointer += 2;
      }
    }
    else
    {
      query_new[right_pointer] = query[left_pointer];
      right_pointer++;
    }
  }

  query_new[right_pointer] = '\0';
  postgres_log(LOG_DEBUG, "Кавычки экранированы");
}

bool is_sql_injection(const char* input)
{
  if (input == NULL)
  {
    return false;
  }

  for (int i = 0; sql_injection_patterns[i] != NULL; i++)
  {
    regex_t regex;
    if (regcomp(&regex, sql_injection_patterns[i], REG_EXTENDED | REG_ICASE) ==
        0)
    {
      if (regexec(&regex, input, 0, NULL, 0) == 0)
      {
        regfree(&regex);
        return true;
      }
      regfree(&regex);
    }
  }

  if (strstr(input, ";") != NULL)
  {
    int in_quotes = 0;
    for (size_t i = 0; input[i] != '\0'; i++)
    {
      if (input[i] == '\'' && (i == 0 || input[i - 1] != '\\'))
      {
        in_quotes = !in_quotes;
      }
      else if (input[i] == ';' && !in_quotes)
      {
        return true;
      }
    }
  }

  return false;
}

// ==================== ПОДКЛЮЧЕНИЕ К БАЗЕ ДАННЫХ ====================

Connection* connection_create(char* parameters)
{
  Connection* connection = (Connection*)malloc(sizeof(Connection));
  if (connection == NULL)
  {
    postgres_log(LOG_ERROR, "Ошибка выделения памяти для подключения");
    return NULL;
  }

  connection->connection = PQconnectdb(parameters);
  connection->log_level = LOG_INFO;
  connection->in_transaction = false;

  postgres_log(LOG_INFO, "Создано подключение к базе данных");
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
  postgres_log(LOG_ERROR, "Аварийное завершение: %s",
               get_error_message(connection));
  finish_connection(connection);
  exit(1);
}

void finish_connection(Connection* connection)
{
  if (connection)
  {
    if (connection->in_transaction)
    {
      postgres_log(LOG_WARNING,
                   "Завершение подключения при активной транзакции");
      rollback_transaction(connection);
    }

    if (connection->connection)
    {
      PQfinish(connection->connection);
      connection->connection = NULL;
    }

    free(connection);
    postgres_log(LOG_INFO, "Подключение закрыто");
  }
}

// ==================== ФАБРИЧНЫЕ МЕТОДЫ ====================

QueryParams* create_string_param(const char* value)
{
  QueryParams* params = create_query_params(1);
  if (params)
  {
    add_string_param(params, 0, value);
  }
  return params;
}

QueryParams* create_int_param(int value)
{
  QueryParams* params = create_query_params(1);
  if (params)
  {
    add_int_param(params, 0, value);
  }
  return params;
}

QueryParams* create_query_params(int count)
{
  if (count <= 0)
  {
    postgres_log(LOG_ERROR, "Некорректное количество параметров: %d", count);
    return NULL;
  }

  QueryParams* params = (QueryParams*)malloc(sizeof(QueryParams));
  if (params == NULL)
  {
    postgres_log(LOG_ERROR, "Ошибка выделения памяти для параметров");
    return NULL;
  }

  params->count = count;
  params->values = (const char**)calloc(count, sizeof(char*));
  params->lengths = (int*)calloc(count, sizeof(int));
  params->formats = (int*)calloc(count, sizeof(int));

  if (params->values == NULL || params->lengths == NULL ||
      params->formats == NULL)
  {
    postgres_log(LOG_ERROR, "Ошибка выделения памяти для массивов параметров");
    free((void*)params->values);
    free(params->lengths);
    free(params->formats);
    free(params);
    return NULL;
  }

  for (int i = 0; i < count; i++)
  {
    params->values[i] = NULL;
    params->lengths[i] = 0;
    params->formats[i] = 0;  // 0 = text, 1 = binary
  }

  postgres_log(LOG_DEBUG, "Создан QueryParams с %d параметрами", count);
  return params;
}

void add_string_param(QueryParams* params, int index, const char* value)
{
  if (params == NULL || index < 0 || index >= params->count)
  {
    postgres_log(LOG_ERROR, "Некорректный индекс параметра: %d", index);
    return;
  }

  if (is_sql_injection(value))
  {
    postgres_log(LOG_SECURITY, "Обнаружена SQL-инъекция в параметре %d: %s",
                 index, value);
    printf(
      "ВНИМАНИЕ: Обнаружена попытка SQL-инъекции! Параметр будет "
      "экранирован.\n");
  }

  if (params->values[index])
  {
    free((void*)params->values[index]);
  }

  size_t len = strlen(value);
  char* new_value = (char*)malloc(len + 1);
  if (new_value == NULL)
  {
    postgres_log(LOG_ERROR, "Ошибка выделения памяти для строкового параметра");
    return;
  }

  strcpy(new_value, value);
  params->values[index] = new_value;
  params->lengths[index] = (int)len;
  params->formats[index] = 0;  // Text format

  postgres_log(LOG_DEBUG, "Добавлен строковый параметр [%d]: %s", index, value);
}

void add_int_param(QueryParams* params, int index, int value)
{
  if (params == NULL || index < 0 || index >= params->count)
  {
    postgres_log(LOG_ERROR, "Некорректный индекс параметра: %d", index);
    return;
  }

  if (params->values[index])
  {
    free((void*)params->values[index]);
  }

  const char BUFFER_SIZE = 20;
  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer), "%d", value);
  size_t len = strlen(buffer);

  char* new_value = (char*)malloc(len + 1);
  if (new_value == NULL)
  {
    postgres_log(LOG_ERROR,
                 "Ошибка выделения памяти для целочисленного параметра");
    return;
  }

  strcpy(new_value, buffer);
  params->values[index] = new_value;
  params->lengths[index] = (int)len;
  params->formats[index] = 0;  // Text format

  postgres_log(LOG_DEBUG, "Добавлен целочисленный параметр [%d]: %d", index,
               value);
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
    free((void*)params->values);
    free(params->lengths);
    free(params->formats);
    free(params);
  }
}

// ==================== ЕДИНЫЙ ИНТЕРФЕЙС ДЛЯ ЗАПРОСОВ ====================

static Oid get_oid_for_type(const char* value)
{
  if (value == NULL)
  {
    return 0;
  }

  bool is_int = true;
  for (size_t i = 0; i < strlen(value); i++)
  {
    if (!isdigit(value[i]) && !(i == 0 && value[i] == '-'))
    {
      is_int = false;
      break;
    }
  }

  return is_int ? INT4OID : TEXTOID;
}

int execute_query_with_permission(Connection* connection, UserSession* session,
                                  QueryType type, const char* query,
                                  QueryParams* params)
{
  if (!has_permission(session, type, NULL))
  {
    return 0;
  }

  const int BUFFER_SIZE = 256;
  char log_details[BUFFER_SIZE];
  snprintf(log_details, sizeof(log_details), "Выполнение запроса типа %d",
           type);
  log_operation(session, "QUERY_EXECUTION", log_details);

  int result = execute_query(connection, type, query, params);

  if (result)
  {
    log_operation(session, "QUERY_SUCCESS", "Запрос выполнен успешно");
  }
  else
  {
    log_operation(session, "QUERY_FAILURE", "Ошибка выполнения запроса");
  }

  return result;
}

int execute_query(Connection* connection, QueryType type, const char* query,
                  QueryParams* params)
{
  if (connection == NULL || query == NULL)
  {
    postgres_log(LOG_ERROR, "Некорректные аргументы для execute_query");
    return 0;
  }

  if (is_sql_injection(query))
  {
    log_sql_injection_attempt(query, "N/A (сам запрос)");
    printf("ВНИМАНИЕ: Запрос содержит потенциально опасные конструкции!\n");
  }

  PGresult* result = NULL;

  if (params == NULL)
  {
    result = PQexec(connection->connection, query);
  }
  else
  {
    Oid* paramTypes = NULL;
    if (params->count > 0)
    {
      paramTypes = (Oid*)malloc(params->count * sizeof(Oid));
      for (int i = 0; i < params->count; i++)
      {
        paramTypes[i] = get_oid_for_type(params->values[i]);
      }
    }

    result =
      PQexecParams(connection->connection, query, params->count, paramTypes,
                   params->values, params->lengths, params->formats, 0);

    free(paramTypes);
  }

  ExecStatusType status = PQresultStatus(result);
  int success = 0;

  switch (type)
  {
    case QUERY_SELECT:
      success = (status == PGRES_TUPLES_OK);
      break;
    case QUERY_INSERT:
    case QUERY_UPDATE:
    case QUERY_DELETE:
      success = (status == PGRES_COMMAND_OK);
      break;
    default:
      success = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);
      break;
  }

  if (!success)
  {
    postgres_log(LOG_ERROR, "Ошибка выполнения запроса: %s",
                 PQerrorMessage(connection->connection));
    printf("Ошибка выполнения запроса: %s\n",
           PQerrorMessage(connection->connection));
  }
  else
  {
    const char* type_str = "неизвестный";
    switch (type)
    {
      case QUERY_SELECT:
        type_str = "SELECT";
        break;
      case QUERY_INSERT:
        type_str = "INSERT";
        break;
      case QUERY_UPDATE:
        type_str = "UPDATE";
        break;
      case QUERY_DELETE:
        type_str = "DELETE";
        break;
      default:
        type_str = "другой";
        break;
    }
    postgres_log(LOG_INFO, "Запрос %s выполнен успешно", type_str);

    if (type == QUERY_INSERT || type == QUERY_UPDATE || type == QUERY_DELETE)
    {
      printf("Затронуто строк: %s\n", PQcmdTuples(result));
    }
  }

  PQclear(result);
  return success;
}

// Небезопасный метод (для демонстрации)
void print_query(Connection* connection, const char* query)
{
  if (is_sql_injection(query))
  {
    log_sql_injection_attempt(query, "N/A");
    printf("ПРЕДУПРЕЖДЕНИЕ: Запрос может быть уязвим к SQL-инъекциям!\n");
  }

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

  printf("Результат (%d строк, %d столбцов):\n", rows, columns);

  for (int i = 0; i < columns; i++)
  {
    printf("%-20s", PQfname(result, i));
  }
  printf("\n");

  for (int i = 0; i < columns; i++)
  {
    printf("%-20s", "--------------------");
  }
  printf("\n");

  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < columns; j++)
    {
      printf("%-20s", PQgetvalue(result, i, j));
    }
    printf("\n");
  }
  printf("\n");

  PQclear(result);
}

// Безопасный метод
void print_query_params(Connection* connection, const char* query,
                        QueryParams* params)
{
  if (is_sql_injection(query))
  {
    log_sql_injection_attempt(query, "N/A");
    printf("ВНИМАНИЕ: Шаблон запроса содержит подозрительные конструкции!\n");
  }

  execute_query(connection, QUERY_SELECT, query, params);
}

// ==================== ОБРАБОТКА ТРАНЗАКЦИЙ ====================

int begin_transaction(Connection* connection)
{
  if (connection->in_transaction)
  {
    postgres_log(LOG_WARNING, "Транзакция уже начата");
    return 0;
  }

  PGresult* result = PQexec(connection->connection, "BEGIN");
  int success = (PQresultStatus(result) == PGRES_COMMAND_OK);

  if (success)
  {
    connection->in_transaction = true;
    postgres_log(LOG_INFO, "Транзакция начата");
  }
  else
  {
    postgres_log(LOG_ERROR, "Не удалось начать транзакцию: %s",
                 PQerrorMessage(connection->connection));
  }

  PQclear(result);
  return success;
}

int commit_transaction(Connection* connection)
{
  if (!connection->in_transaction)
  {
    postgres_log(LOG_WARNING, "Нет активной транзакции для коммита");
    return 0;
  }

  PGresult* result = PQexec(connection->connection, "COMMIT");
  int success = (PQresultStatus(result) == PGRES_COMMAND_OK);

  if (success)
  {
    connection->in_transaction = false;
    postgres_log(LOG_INFO, "Транзакция закоммичена");
  }
  else
  {
    postgres_log(LOG_ERROR, "Не удалось закоммитить транзакцию: %s",
                 PQerrorMessage(connection->connection));
  }

  PQclear(result);
  return success;
}

int rollback_transaction(Connection* connection)
{
  if (!connection->in_transaction)
  {
    postgres_log(LOG_WARNING, "Нет активной транзакции для отката");
    return 0;
  }

  PGresult* result = PQexec(connection->connection, "ROLLBACK");
  int success = (PQresultStatus(result) == PGRES_COMMAND_OK);

  if (success)
  {
    connection->in_transaction = false;
    postgres_log(LOG_INFO, "Транзакция откачена");
  }
  else
  {
    postgres_log(LOG_ERROR, "Не удалось откатить транзакцию: %s",
                 PQerrorMessage(connection->connection));
  }

  PQclear(result);
  return success;
}

int execute_in_transaction(Connection* connection, const char* query,
                           QueryParams* params)
{
  if (!begin_transaction(connection))
  {
    return 0;
  }

  int success = execute_query(connection, QUERY_OTHER, query, params);

  if (success)
  {
    success = commit_transaction(connection);
  }
  else
  {
    rollback_transaction(connection);
  }

  return success;
}

// ==================== ПОДГОТОВЛЕННЫЕ ВЫРАЖЕНИЯ ====================

PreparedStatement* prepare_statement(Connection* connection, const char* name,
                                     const char* query)
{
  if (is_sql_injection(query))
  {
    log_sql_injection_attempt(query, "N/A (подготовленное выражение)");
    printf(
      "ВНИМАНИЕ: Подготовленное выражение содержит опасные конструкции!\n");
    return NULL;
  }

  int param_count = 0;
  for (const char* param = query; *param != '\0'; param++)
  {
    if (*param == '$' && isdigit(*(param + 1)))
    {
      const int BASE = 10;
      int num = (int)strtol(param + 1, NULL, BASE);
      if (num > param_count)
      {
        param_count = num;
      }
    }
  }

  PreparedStatement* statement =
    (PreparedStatement*)malloc(sizeof(PreparedStatement));
  if (statement == NULL)
  {
    postgres_log(LOG_ERROR,
                 "Ошибка выделения памяти для подготовленного выражения");
    return NULL;
  }

  statement->name = strdup(name);
  statement->query = strdup(query);
  statement->n_params = param_count;

  PGresult* result =
    PQprepare(connection->connection, name, query, param_count, NULL);

  if (PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    postgres_log(LOG_ERROR, "Не удалось подготовить выражение: %s",
                 PQerrorMessage(connection->connection));
    free(statement->name);
    free(statement->query);
    free(statement);
    statement = NULL;
  }
  else
  {
    postgres_log(LOG_INFO, "Подготовлено выражение '%s' с %d параметрами", name,
                 param_count);
  }

  PQclear(result);
  return statement;
}

int execute_prepared(Connection* connection, const char* name,
                     QueryParams* params)
{
  if (connection == NULL || name == NULL)
  {
    return 0;
  }

  PGresult* result = PQexecPrepared(
    connection->connection, name, params ? params->count : 0,
    params ? params->values : NULL, params ? params->lengths : NULL,
    params ? params->formats : NULL, 0);

  int success = (PQresultStatus(result) == PGRES_TUPLES_OK ||
                 PQresultStatus(result) == PGRES_COMMAND_OK);

  if (success)
  {
    postgres_log(LOG_INFO, "Выполнено подготовленное выражение '%s'", name);
  }
  else
  {
    postgres_log(LOG_ERROR, "Ошибка выполнения подготовленного выражения: %s",
                 PQerrorMessage(connection->connection));
  }

  PQclear(result);
  return success;
}

void free_prepared_statement(PreparedStatement* statement)
{
  if (statement)
  {
    free(statement->name);
    free(statement->query);
    free(statement);
    postgres_log(LOG_DEBUG, "Подготовленное выражение освобождено");
  }
}

// ==================== ПРЕДСТАВЛЕНИЯ ====================
int create_view(Connection* connection, const char* view_name,
                const char* query)
{
  if (is_sql_injection(view_name) || is_sql_injection(query))
  {
    postgres_log(LOG_SECURITY,
                 "Обнаружены опасные конструкции при создании представления");
    return 0;
  }

  const int BUFFER_SIZE = 1024;
  char create_sql[BUFFER_SIZE];
  snprintf(create_sql, sizeof(create_sql), "CREATE OR REPLACE VIEW %s AS %s",
           view_name, query);

  return execute_query(connection, QUERY_OTHER, create_sql, NULL);
}

int drop_view(Connection* connection, const char* view_name)
{
  if (is_sql_injection(view_name))
  {
    postgres_log(LOG_SECURITY,
                 "Обнаружены опасные конструкции при удалении представления");
    return 0;
  }

  const int BUFFER_SIZE = 256;
  char drop_sql[BUFFER_SIZE];
  snprintf(drop_sql, sizeof(drop_sql), "DROP VIEW IF EXISTS %s", view_name);

  return execute_query(connection, QUERY_OTHER, drop_sql, NULL);
}

int materialize_view(Connection* connection, const char* view_name)
{
  if (is_sql_injection(view_name))
  {
    postgres_log(
      LOG_SECURITY,
      "Обнаружены опасные конструкции при материализации представления");
    return 0;
  }

  const int BUFFER_SIZE = 512;
  char materialize_sql[BUFFER_SIZE];
  snprintf(materialize_sql, sizeof(materialize_sql),
           "CREATE MATERIALIZED VIEW IF NOT EXISTS %s_materialized AS SELECT * "
           "FROM %s",
           view_name, view_name);

  return execute_query(connection, QUERY_OTHER, materialize_sql, NULL);
}

// ==================== CRUD ОПЕРАЦИИ ДЛЯ ОЦЕНОК ====================

int add_grade(Connection* conn, UserSession* session, int student_id,
              const char* field_name, int mark)
{
  if (!has_permission(session, QUERY_INSERT, "field_comprehensions"))
  {
    printf("У вас нет прав на добавление оценок!\n");
    return 0;
  }

  if (is_sql_injection(field_name))
  {
    log_sql_injection_attempt("INSERT grade", field_name);
    printf("Обнаружена попытка SQL-инъекции!\n");
    return 0;
  }

  char* query =
    "INSERT INTO field_comprehensions (student_id, field, mark) "
    "VALUES ($1, "
    "(SELECT field_id FROM fields WHERE field_name = $2 LIMIT 1), $3) "
    "ON CONFLICT (student_id, field) DO UPDATE SET mark = EXCLUDED.mark";

  QueryParams* params = create_query_params(3);
  add_int_param(params, 0, student_id);
  add_string_param(params, 1, field_name);
  add_int_param(params, 2, mark);

  int result =
    execute_query_with_permission(conn, session, QUERY_INSERT, query, params);

  if (result)
  {
    printf("✓ Оценка успешно добавлена/обновлена\n");
    printf("  Студент: %d, Дисциплина: %s, Оценка: %d\n", student_id,
           field_name, mark);
  }

  free_query_params(params);
  return result;
}

int update_grade(Connection* conn, UserSession* session, int student_id,
                 const char* field_name, int new_mark)
{
  if (!has_permission(session, QUERY_UPDATE, "field_comprehensions"))
  {
    printf("У вас нет прав на изменение оценок!\n");
    return 0;
  }

  if (is_sql_injection(field_name))
  {
    log_sql_injection_attempt("UPDATE grade", field_name);
    printf("Обнаружена попытка SQL-инъекции!\n");
    return 0;
  }

  char* query =
    "UPDATE field_comprehensions fc "
    "SET mark = $1 "
    "WHERE student_id = $2 "
    "AND field = (SELECT field_id FROM fields WHERE field_name = $3 LIMIT 1)";

  QueryParams* params = create_query_params(3);
  add_int_param(params, 0, new_mark);
  add_int_param(params, 1, student_id);
  add_string_param(params, 2, field_name);

  int result =
    execute_query_with_permission(conn, session, QUERY_UPDATE, query, params);

  if (result)
  {
    printf("✓ Оценка успешно обновлена\n");
    printf("  Студент: %d, Дисциплина: %s, Новая оценка: %d\n", student_id,
           field_name, new_mark);
  }

  free_query_params(params);
  return result;
}

int delete_grade(Connection* conn, UserSession* session, int student_id,
                 const char* field_name)
{
  if (!has_permission(session, QUERY_DELETE, "field_comprehensions"))
  {
    printf("У вас нет прав на удаление оценок!\n");
    return 0;
  }

  if (is_sql_injection(field_name))
  {
    log_sql_injection_attempt("DELETE grade", field_name);
    printf("Обнаружена попытка SQL-инъекции!\n");
    return 0;
  }

  char* query =
    "DELETE FROM field_comprehensions fc "
    "WHERE student_id = $1 "
    "AND field = (SELECT field_id FROM fields WHERE field_name = $2 LIMIT 1)";

  QueryParams* params = create_query_params(2);
  add_int_param(params, 0, student_id);
  add_string_param(params, 1, field_name);

  int result =
    execute_query_with_permission(conn, session, QUERY_DELETE, query, params);

  if (result)
  {
    printf("✓ Оценка успешно удалена\n");
    printf("  Студент: %d, Дисциплина: %s\n", student_id, field_name);
  }

  free_query_params(params);
  return result;
}

int get_student_grades(Connection* conn, UserSession* session, int student_id)
{
  if (!has_permission(session, QUERY_SELECT, "field_comprehensions"))
  {
    printf("У вас нет прав на просмотр оценок!\n");
    return 0;
  }

  char* query =
    "SELECT s.last_name, s.first_name, f.field_name, fc.mark "
    "FROM students s "
    "JOIN field_comprehensions fc ON s.student_id = fc.student_id "
    "JOIN fields f ON fc.field = f.field_id "
    "WHERE s.student_id = $1 "
    "ORDER BY f.field_name";

  QueryParams* params = create_int_param(student_id);

  printf("Оценки студента (ID: %d):\n", student_id);
  printf("============================\n");

  int result =
    execute_query_with_permission(conn, session, QUERY_SELECT, query, params);

  free_query_params(params);
  return result;
}

int search_student_grades(Connection* conn, UserSession* session,
                          const char* last_name, const char* first_name,
                          const char* group_number)
{
  if (!has_permission(session, QUERY_SELECT, "field_comprehensions"))
  {
    printf("У вас нет прав на поиск оценок!\n");
    return 0;
  }

  if (is_sql_injection(last_name) || is_sql_injection(first_name) ||
      is_sql_injection(group_number))
  {
    log_sql_injection_attempt("SEARCH grades", last_name);
    printf("Обнаружена попытка SQL-инъекции!\n");
    return 0;
  }

  char* query =
    "SELECT s.student_id, s.last_name, s.first_name, s.students_group_number, "
    "f.field_name, fc.mark "
    "FROM students s "
    "JOIN field_comprehensions fc ON s.student_id = fc.student_id "
    "JOIN fields f ON fc.field = f.field_id "
    "WHERE s.last_name LIKE $1 "
    "AND s.first_name LIKE $2 "
    "AND s.students_group_number LIKE $3 "
    "ORDER BY s.last_name, s.first_name, f.field_name";

  QueryParams* params = create_query_params(3);

  const int BUFFER_SIZE = 256;
  char safe_last_name[BUFFER_SIZE];
  char safe_first_name[BUFFER_SIZE];
  char safe_group[BUFFER_SIZE];

  snprintf(safe_last_name, sizeof(safe_last_name), "%%%s%%", last_name);
  snprintf(safe_first_name, sizeof(safe_first_name), "%%%s%%", first_name);
  snprintf(safe_group, sizeof(safe_group), "%%%s%%", group_number);

  add_string_param(params, 0, safe_last_name);
  add_string_param(params, 1, safe_first_name);
  add_string_param(params, 2, safe_group);

  printf("Результаты поиска (%s %s, группа: %s):\n", last_name, first_name,
         group_number);
  printf("========================================\n");

  int result =
    execute_query_with_permission(conn, session, QUERY_SELECT, query, params);

  free_query_params(params);
  return result;
}

int get_my_grades(Connection* conn, UserSession* session)
{
  if (!has_permission(session, QUERY_SELECT, "field_comprehensions"))
  {
    return 0;
  }

  if (session->student_id == -1)
  {
    printf("Для вашего пользователя не найден ID студента.\n");
    printf("Используйте поиск по фамилии и имени.\n");
    return 0;
  }

  return get_student_grades(conn, session, session->student_id);
}
