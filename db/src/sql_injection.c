#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "postgres.h"
#include "types.h"

#define DELIMETER "-------------\n"

void unsafe_find_student(Connection* conn, int student_id,
                         const char* field_name)
{
  printf("Опасный запрос\n%s", DELIMETER);

  const int QUERY_LENGTH_LIMIT = 512;
  char query[QUERY_LENGTH_LIMIT];
  sprintf(
    query,
    "SELECT s.student_id, s.last_name, s.first_name, f.field_name, fc.mark "
    "FROM students s "
    "JOIN field_comprehensions fc ON s.student_id = fc.student_id "
    "JOIN fields f ON fc.field = f.field_id "
    "WHERE s.student_id = %d AND f.field_name = '%s' "
    "LIMIT 5",
    student_id, field_name);

  printf("Выполняемый запрос:\n%s\n", query);
  print_query(conn, query);
}

// Safe request
void safe_find_student(Connection* conn, int student_id, const char* field_name)
{
  printf("Безопасный запрос\n%s", DELIMETER);

  char* query =
    "SELECT s.student_id, s.last_name, s.first_name, f.field_name, fc.mark "
    "FROM students s "
    "JOIN field_comprehensions fc ON s.student_id = fc.student_id "
    "JOIN fields f ON fc.field = f.field_id "
    "WHERE s.student_id = $1 AND f.field_name = $2 "
    "LIMIT 5";

  printf("Шаблон запроса:\n%s\n", query);
  printf("Параметры: student_id=%d, field_name='%s'\n", student_id, field_name);

  QueryParams* params = create_query_params(2);
  add_int_param(params, 0, student_id);
  add_string_param(params, 1, field_name);

  print_query_params(conn, query, params);
  free_query_params(params);
}

void test_sql_injection(Connection* conn)
{
  printf("%sТестирование защиты от SQL-инъекций\n%s", DELIMETER, DELIMETER);

  const int student_id = 844688;
  const char* target_field = "Операционные системы";

  printf("1. Обычный запрос:\n%s", DELIMETER);
  unsafe_find_student(conn, student_id, target_field);
  safe_find_student(conn, student_id, target_field);

  printf("2. SQL-инъекция (попытка получить все записи):\n%s", DELIMETER);
  char first_injection[] = "Операционные системы' OR '1'='1";
  unsafe_find_student(conn, student_id, first_injection);
  safe_find_student(conn, student_id, first_injection);

  printf(
    "3. SQL-инъекция (попытка изменить оценку по конкретному предмету):\n%s",
    DELIMETER);
  char second_injection[] =
    "1'; UPDATE field_comprehensions SET mark = 5 "
    "FROM fields f "
    "WHERE field_comprehensions.field = f.field_id "
    "AND field_comprehensions.student_id = 844688 "
    "AND f.field_name = 'Операционные системы'; --";
  unsafe_find_student(conn, student_id, second_injection);
  safe_find_student(conn, student_id, second_injection);

  printf("4. Проверка: оценка не должна была измениться\n%s", DELIMETER);
  safe_find_student(conn, student_id, target_field);
}

int main()
{
  printf("Устанавливаем соединение с PostgreSQL...\n");

  char* connection_parameters =
    "host=localhost "
    "port=5432 "
    "user=admin "
    "password=123456 "
    "dbname=enhanced_students";

  Connection* connection = connection_create(connection_parameters);

  if (get_connection_status(connection) == CONNECTION_STATUS_BAD)
  {
    printf("Не удалось установить соединение с базой данных: %s\n",
           get_error_message(connection));
    exit_with_error(connection);
  }

  printf("Соединение с БД установлено успешно!\n");

  test_sql_injection(connection);

  printf("\nВсе тесты завершены!\n");
  finish_connection(connection);

  return 0;
}
