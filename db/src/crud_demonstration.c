#include <stdio.h>

#include "postgres.h"
#include "types.h"

void demonstrate_crud_operations(Connection* conn)
{
  printf("\n%sДемонстрация CRUD операций%s\n",
         "========================================\n",
         "========================================\n");

  // CREATE - Создание временной таблицы
  printf("1. CREATE - Создание временной таблицы:\n");
  execute_query(conn, QUERY_OTHER,
                "CREATE TEMP TABLE IF NOT EXISTS test_crud ("
                "id SERIAL PRIMARY KEY, "
                "name TEXT NOT NULL, "
                "value INTEGER)",
                NULL);

  // INSERT - Вставка данных
  printf("\n2. INSERT - Вставка данных:\n");
  QueryParams* insert_params = create_query_params(2);
  add_string_param(insert_params, 0, "Тестовая запись");
  add_int_param(insert_params, 1, 100);

  execute_query(conn, QUERY_INSERT,
                "INSERT INTO test_crud (name, value) VALUES ($1, $2)",
                insert_params);

  // Вставка еще одной записи
  add_string_param(insert_params, 0, "Еще одна запись");
  add_int_param(insert_params, 1, 200);
  execute_query(conn, QUERY_INSERT,
                "INSERT INTO test_crud (name, value) VALUES ($1, $2)",
                insert_params);

  free_query_params(insert_params);

  // READ - Чтение данных
  printf("\n3. READ - Чтение данных:\n");
  execute_query(conn, QUERY_SELECT, "SELECT * FROM test_crud ORDER BY id",
                NULL);

  // UPDATE - Обновление данных
  printf("\n4. UPDATE - Обновление данных:\n");
  QueryParams* update_params = create_query_params(2);
  add_int_param(update_params, 0, 150);  // Новое значение
  add_int_param(update_params, 1, 1);    // ID записи

  execute_query(conn, QUERY_UPDATE,
                "UPDATE test_crud SET value = $1 WHERE id = $2", update_params);

  printf("После обновления:\n");
  execute_query(conn, QUERY_SELECT, "SELECT * FROM test_crud ORDER BY id",
                NULL);

  free_query_params(update_params);

  // DELETE - Удаление данных
  printf("\n5. DELETE - Удаление данных:\n");
  QueryParams* delete_params = create_int_param(1);

  execute_query(conn, QUERY_DELETE, "DELETE FROM test_crud WHERE id = $1",
                delete_params);

  printf("После удаления:\n");
  execute_query(conn, QUERY_SELECT, "SELECT * FROM test_crud ORDER BY id",
                NULL);

  free_query_params(delete_params);

  printf("\nCRUD операции завершены успешно!\n");
}

void demonstrate_unified_interface(Connection* conn)
{
  printf("\n%sЕдиный интерфейс execute_query%s\n",
         "========================================\n",
         "========================================\n");

  printf("Использование единого интерфейса для разных типов запросов:\n\n");

  // SELECT запрос
  printf("1. SELECT запрос:\n");
  QueryParams* select_params = create_query_params(1);
  add_int_param(select_params, 0, 844688);

  execute_query(conn, QUERY_SELECT,
                "SELECT student_id, last_name, first_name "
                "FROM students WHERE student_id = $1",
                select_params);

  free_query_params(select_params);

  // INSERT запрос в транзакции
  printf("\n2. INSERT запрос в транзакции:\n");
  begin_transaction(conn);

  QueryParams* insert_params = create_query_params(2);
  add_string_param(insert_params, 0, "Новый студент");
  add_string_param(insert_params, 1, "Тест");

  execute_query(conn, QUERY_INSERT,
                "INSERT INTO test_crud (name, value) VALUES ($1, 999)",
                insert_params);

  commit_transaction(conn);
  free_query_params(insert_params);

  printf("\n3. Проверка результата INSERT:\n");
  execute_query(conn, QUERY_SELECT, "SELECT * FROM test_crud WHERE value = 999",
                NULL);
}

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

  // Устанавливаем уровень логирования
  set_log_level(LOG_INFO);

  print_connection_status(connection);

  if (get_connection_status(connection) == CONNECTION_STATUS_BAD)
  {
    printf("Не удалось установить соединение с базой данных: %s\n",
           get_error_message(connection));
    exit_with_error(connection);
  }

  printf("Соединение с БД успешно установлено!\n\n");

  // Тестовые запросы
  printf("Выполняем тестовый запрос...\n");
  print_query(connection, "SELECT version()");

  printf("Первые 5 студентов:\n");
  print_query(connection, "SELECT * FROM students LIMIT 5;");

  // Демонстрация новых возможностей
  demonstrate_crud_operations(connection);
  demonstrate_unified_interface(connection);

  printf("\nВсе операции завершены успешно!\n");
  finish_connection(connection);

  return 0;
}
