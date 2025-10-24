#include <stdio.h>

#include "postgres.h"
#include "types.h"

int main()
{
  printf("Устанавливаем соединение с PostgreSQL...\n");

  char* connectionParameters =
    "host=localhost "
    "port=5432 "
    "user=admin "
    "password=123456 "
    "dbname=enhanced_students";

  Connection* connection = connection_create(connectionParameters);

  print_connection_status(connection);

  if (get_connection_status(connection) == CONNECTION_STATUS_BAD)
  {
    int print_result =
      printf("Не удалось установить соединение с базой данных: %s\n",
             get_error_message(connection));
    exit_with_error(connection);
  }

  printf("Соединение с БД успешно установлено! Выполняем тестовый запрос...\n");

  print_query(connection, "SELECT version()");
  print_query(connection, "SELECT * FROM students LIMIT 5;");

  printf("Все операции завершены успешно!");
  finish_connection(connection);

  return 0;
}
