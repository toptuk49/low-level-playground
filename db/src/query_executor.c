#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "postgres.h"
#include "types.h"

// Запрос из ЛР 3 (Запрос 53)
// Выводит ФИО преподавателей и руководителей в одной колонке
void execute_professors_and_heads_query(Connection* conn)
{
  printf("\n%sЗапрос из Лабораторной работы 3%s\n",
         "========================================\n",
         "========================================\n");

  printf("Запрос 53: Вывести ФИО всех преподавателей и руководителей\n");
  printf("структурных подразделений в одной колонке.\n\n");

  char* query =
    "SELECT "
    "    p.last_name || ' ' || p.first_name || ' ' || COALESCE(p.patronymic, "
    "'') AS full_name, "
    "    'преподаватель' AS role "
    "FROM professors p "
    "UNION ALL "
    "SELECT "
    "    su.head_of_the_unit AS full_name, "
    "    'руководитель' AS role "
    "FROM structural_units su "
    "ORDER BY role, full_name "
    "LIMIT 20";

  printf("Используем параметризованный запрос для безопасности:\n");
  printf("----------------------------------------\n");

  execute_query(conn, QUERY_SELECT, query, NULL);

  printf("\nЗапрос выполнен с защитой от SQL-инъекций!\n");
}

// Запрос из ЛР 2 (Запрос 33)
// Средняя оценка по каждой дисциплине (более 3.5)
void execute_average_marks_query(Connection* conn)
{
  printf("\n%sЗапрос из Лабораторной работы 2%s\n",
         "========================================\n",
         "========================================\n");

  printf("Запрос 33: Средняя оценка по каждой дисциплине (только > 3.5)\n\n");

  char* query =
    "WITH avg_mark AS ( "
    "    SELECT "
    "        f.field_name, "
    "        s.last_name || ' ' || s.first_name || ' ' || "
    "COALESCE(s.patronymic, '') AS full_name, "
    "        AVG(fc.mark) OVER (PARTITION BY f.field_name) AS avg_mark "
    "    FROM students s "
    "    JOIN field_comprehensions fc ON s.student_id = fc.student_id "
    "    JOIN fields f ON fc.field = f.field_id "
    "    WHERE s.students_group_number ~ $1 "
    "    AND fc.mark IS NOT NULL "
    ") "
    "SELECT field_name, full_name, ROUND(avg_mark::numeric, 2) AS average_mark "
    "FROM avg_mark "
    "WHERE avg_mark > $2 "
    "ORDER BY avg_mark DESC "
    "LIMIT 10";

  QueryParams* params = create_query_params(2);
  add_string_param(params, 0, "^[A-Яа-я]+-3\\d$");
  add_string_param(params, 1, "3.5");

  printf("Выполняем безопасный запрос с параметрами:\n");
  printf("----------------------------------------\n");
  printf("Параметр 1 (регулярное выражение): %s\n", params->values[0]);
  printf("Параметр 2 (минимальная оценка): %s\n", params->values[1]);

  execute_query(conn, QUERY_SELECT, query, params);
  free_query_params(params);
}

// Запрос из ЛР 6 (Запрос 3)
// Средний балл по каждой дисциплине у преподавателей
void execute_professor_grades_query(Connection* conn)
{
  printf("\n%sЗапрос из Лабораторной работы 6%s\n",
         "========================================\n",
         "========================================\n");

  printf("Запрос 3: Средний балл по каждой дисциплине у преподавателей\n\n");

  char* query =
    "SELECT "
    "    p.last_name || ' ' || p.first_name || ' ' || COALESCE(p.patronymic, "
    "'') AS professor_name, "
    "    f.field_name AS discipline, "
    "    ROUND(AVG(fc.mark)::numeric, 2) AS average_grade "
    "FROM professors p "
    "INNER JOIN professor_fields pf ON pf.professor_id = p.professor_id "
    "INNER JOIN fields f ON f.field_id = pf.field_id "
    "INNER JOIN field_comprehensions fc ON fc.field = f.field_id "
    "WHERE p.last_name LIKE $1 "
    "GROUP BY p.last_name, p.first_name, p.patronymic, f.field_name "
    "ORDER BY professor_name, average_grade DESC "
    "LIMIT 15";

  printf("Пример безопасного запроса с фильтрацией по фамилии:\n");
  printf("----------------------------------------\n");

  QueryParams* params = create_query_params(1);
  add_string_param(params, 0, "%ов%");

  printf("Поиск преподавателей с фамилией содержащей 'ов':\n");
  execute_query(conn, QUERY_SELECT, query, params);

  printf("\nПроверка защиты от SQL-инъекций:\n");
  printf("----------------------------------------\n");

  char dangerous_input[] = "%ов%' OR '1'='1";
  printf("Попытка ввода: %s\n", dangerous_input);

  if (is_sql_injection(dangerous_input))
  {
    printf("Обнаружена попытка SQL-инъекции! Ввод будет экранирован.\n");

    char escaped_input[256];
    add_escape_quotes(dangerous_input, escaped_input, sizeof(escaped_input));

    printf("Экранированный ввод: %s\n", escaped_input);

    add_string_param(params, 0, escaped_input);
    execute_query(conn, QUERY_SELECT, query, params);
  }

  free_query_params(params);
}

void demonstrate_protection_techniques(Connection* conn)
{
  printf("\n%sДемонстрация методов защиты%s\n",
         "========================================\n",
         "========================================\n");

  printf("1. Параметризованные запросы:\n");
  char* safe_query =
    "SELECT * FROM students WHERE last_name = $1 AND first_name = $2";
  QueryParams* params = create_query_params(2);
  add_string_param(params, 0, "Иванов");
  add_string_param(params, 1, "Иван");
  printf("   Запрос: %s\n", safe_query);
  printf("   Параметры: last_name='%s', first_name='%s'\n", params->values[0],
         params->values[1]);
  free_query_params(params);

  printf("\n2. Метод замены кавычек на апострофы:\n");
  char test_query[] = "SELECT * FROM users WHERE name = 'O''Brien'";
  printf("   До: %s\n", test_query);
  correct_apostrof(test_query);
  printf("   После: %s\n", test_query);

  printf("\n3. Метод экранирования кавычек:\n");
  const char* original = "INSERT INTO users (name) VALUES ('O''Brien')";
  const int BUFFER_SIZE = 256;
  char escaped[BUFFER_SIZE];
  add_escape_quotes(original, escaped, sizeof(escaped));
  printf("   До: %s\n", original);
  printf("   После: %s\n", escaped);
}

int main()
{
  printf("Запуск программы выполнения SQL запросов\n");
  printf("========================================\n\n");

  char* connection_parameters =
    "host=localhost "
    "port=5432 "
    "user=admin "
    "password=123456 "
    "dbname=enhanced_students";

  Connection* connection = connection_create(connection_parameters);
  set_log_level(LOG_INFO);

  if (get_connection_status(connection) == CONNECTION_STATUS_BAD)
  {
    printf("Не удалось установить соединение с базой данных: %s\n",
           get_error_message(connection));
    exit_with_error(connection);
  }

  printf("Соединение с БД успешно установлено!\n");

  execute_professors_and_heads_query(connection);
  execute_average_marks_query(connection);
  execute_professor_grades_query(connection);
  demonstrate_protection_techniques(connection);

  printf("\n%sВсе запросы выполнены успешно!%s\n",
         "========================================\n",
         "========================================\n");

  finish_connection(connection);
  return 0;
}
