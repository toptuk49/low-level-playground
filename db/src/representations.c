#include <stdio.h>

#include "postgres.h"
#include "types.h"

void demonstrate_views_usage(Connection* conn)
{
  printf("\n%sИспользование представлений%s\n",
         "========================================\n",
         "========================================\n");

  printf("Преимущества использования представлений:\n");
  printf("1. Упрощение сложных запросов\n");
  printf("2. Повторное использование логики\n");
  printf("3. Безопасность (скрытие структуры таблиц)\n");
  printf("4. Абстракция данных\n\n");

  printf("Пример 1: Использование представления из ЛР 3\n");
  printf("----------------------------------------\n");

  // Представление из Лабораторной работы 3
  char* view_query1 = "SELECT * FROM all_units_with_groups LIMIT 10";
  printf("Запрос к представлению 'all_units_with_groups':\n");
  printf("%s\n\n", view_query1);

  printf("Результат:\n");
  execute_query(conn, QUERY_SELECT, view_query1, NULL);

  printf("\nПример 2: Использование представления из ЛР 3\n");
  printf("----------------------------------------\n");

  char* view_query2 = "SELECT * FROM students_without_fields LIMIT 10";
  printf("Запрос к представлению 'students_without_fields':\n");
  printf("%s\n\n", view_query2);

  printf("Результат:\n");
  execute_query(conn, QUERY_SELECT, view_query2, NULL);

  printf("\nСравнение с прямым запросом:\n");
  printf("----------------------------------------\n");

  printf("Без представления (сложный запрос):\n");
  char* complex_query =
    "SELECT "
    "    s.student_id, "
    "    s.last_name || ' ' || s.first_name AS \"ФИО студента\", "
    "    s.students_group_number AS \"Группа\", "
    "    s.email AS \"Почта\" "
    "FROM students s "
    "WHERE NOT EXISTS ("
    "    SELECT 1 FROM field_comprehensions fc "
    "    WHERE fc.student_id = s.student_id"
    ") "
    "ORDER BY \"ФИО студента\" "
    "LIMIT 5";

  printf("%s\n\n", complex_query);
  printf("Результат:\n");
  execute_query(conn, QUERY_SELECT, complex_query, NULL);
}

void create_and_use_custom_view(Connection* conn)
{
  printf("\n%sСоздание и использование представления%s\n",
         "========================================\n",
         "========================================\n");

  printf("Создаем представление 'top_students_by_average':\n");
  printf("----------------------------------------\n");

  begin_transaction(conn);

  char* create_view_sql =
    "CREATE OR REPLACE VIEW top_students_by_average AS "
    "SELECT "
    "    s.student_id, "
    "    s.last_name || ' ' || s.first_name || ' ' || COALESCE(s.patronymic, "
    "'') AS full_name, "
    "    sg.students_group_number, "
    "    ROUND(AVG(fc.mark)::numeric, 2) AS average_mark, "
    "    COUNT(fc.mark) AS marks_count "
    "FROM students s "
    "JOIN students_groups sg ON s.students_group_number = "
    "sg.students_group_number "
    "JOIN field_comprehensions fc ON s.student_id = fc.student_id "
    "WHERE fc.mark IS NOT NULL "
    "GROUP BY s.student_id, s.last_name, s.first_name, s.patronymic, "
    "sg.students_group_number "
    "HAVING AVG(fc.mark) >= 4.0 "
    "ORDER BY average_mark DESC";

  if (execute_query(conn, QUERY_OTHER, create_view_sql, NULL))
  {
    commit_transaction(conn);
    printf("Представление успешно создано!\n\n");

    printf("Используем созданное представление:\n");
    printf("----------------------------------------\n");

    char* use_view_sql = "SELECT * FROM top_students_by_average LIMIT 10";
    execute_query(conn, QUERY_SELECT, use_view_sql, NULL);

    printf("\nПреимущества такого подхода:\n");
    printf("1. Инкапсуляция сложной логики\n");
    printf("2. Возможность изменения представления без изменения кода C\n");
    printf(
      "3. Производительность (представление может быть материализовано)\n");
    printf("4. Безопасность (можно выдавать права только на представление)\n");

    printf("\nНедостатки:\n");
    printf("1. Дополнительный слой абстракции\n");
    printf(
      "2. Возможные проблемы с производительностью при неправильном "
      "использовании\n");
    printf("3. Сложность отладки\n");
  }
  else
  {
    rollback_transaction(conn);
    printf("✗ Ошибка при создании представления\n");
  }
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

  set_log_level(LOG_INFO);

  print_connection_status(connection);

  if (get_connection_status(connection) == CONNECTION_STATUS_BAD)
  {
    printf("Не удалось установить соединение с базой данных: %s\n",
           get_error_message(connection));
    exit_with_error(connection);
  }

  printf("Соединение с БД успешно установлено!\n\n");

  printf("Выполняем тестовый запрос...\n");
  print_query(connection, "SELECT version()");

  printf("Первые 5 студентов:\n");
  print_query(connection, "SELECT * FROM students LIMIT 5;");

  demonstrate_views_usage(connection);

  create_and_use_custom_view(connection);

  printf("\nВсе операции завершены успешно!\n");
  finish_connection(connection);

  return 0;
}
