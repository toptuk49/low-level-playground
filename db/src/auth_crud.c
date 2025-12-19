#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "postgres.h"
#include "types.h"

#define MAX_INPUT 256

void get_password(char* password, size_t max_len)
{
  struct termios oldt;
  struct termios newt;
  int index = 0;
  int current_char;

  printf("Пароль: ");
  fflush(stdout);

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;

  newt.c_lflag &= ~(ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  while ((current_char = getchar()) != '\n' && current_char != EOF &&
         index < max_len - 1)
  {
    password[index++] = (char)current_char;
  }
  password[index] = '\0';

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  printf("\n");
}

UserSession* authenticate(Connection* conn)
{
  char username[MAX_INPUT];
  char password[MAX_INPUT];

  printf("\n%sСистема аутентификации%s\n",
         "========================================\n",
         "========================================\n");

  printf("Доступные пользователи:\n");
  printf("1. super_admin    - Администратор (полные права)\n");
  printf("2. junior   - Младший пользователь (только чтение)\n");
  printf("3. test_user    - Обычный пользователь\n");
  printf("4. admin        - Администратор по умолчанию\n");
  printf("\nИли введите другого существующего пользователя\n");

  printf("\nВведите имя пользователя: ");
  fgets(username, sizeof(username), stdin);
  username[strcspn(username, "\n")] = 0;

  get_password(password, sizeof(password));

  UserSession* session = create_user_session();
  if (session == NULL)
  {
    printf("Ошибка создания сессии!\n");
    return NULL;
  }

  if (authenticate_user(conn, session, username, password))
  {
    printf("\nАутентификация успешна!\n");
    printf("  Пользователь: %s\n", session->username);
    printf("  Роль: %s\n", get_user_role_string(session->role));

    if (session->student_id != -1)
    {
      printf("  ID студента: %d\n", session->student_id);
    }

    return session;
  }

  printf("\nОшибка аутентификации!\n");
  destroy_user_session(session);
  return NULL;
}

void admin_menu(Connection* conn, UserSession* session)
{
  int choice;
  char input[MAX_INPUT];

  do
  {
    printf("\n%sМеню администратора%s\n",
           "========================================\n",
           "========================================\n");

    printf("1. Просмотреть оценки студента\n");
    printf("2. Добавить/обновить оценку\n");
    printf("3. Изменить оценку\n");
    printf("4. Удалить оценку\n");
    printf("5. Поиск оценок по ФИО и группе\n");
    printf("6. Показать все студенты (первые 10)\n");
    printf("7. Показать все дисциплины\n");
    printf("8. Тестирование защиты от SQL-инъекций\n");
    printf("0. Выход\n");

    printf("\nВыберите действие: ");
    fgets(input, sizeof(input), stdin);
    choice = atoi(input);

    switch (choice)
    {
      case 1:
      {
        printf("Введите ID студента: ");
        fgets(input, sizeof(input), stdin);
        int student_id = atoi(input);
        get_student_grades(conn, session, student_id);
        break;
      }

      case 2:
      {
        int student_id, mark;
        char field_name[MAX_INPUT];

        printf("Введите ID студента: ");
        fgets(input, sizeof(input), stdin);
        student_id = atoi(input);

        printf("Введите название дисциплины: ");
        fgets(field_name, sizeof(field_name), stdin);
        field_name[strcspn(field_name, "\n")] = 0;

        printf("Введите оценку (2-5): ");
        fgets(input, sizeof(input), stdin);
        mark = atoi(input);

        add_grade(conn, session, student_id, field_name, mark);
        break;
      }

      case 3:
      {
        int student_id, new_mark;
        char field_name[MAX_INPUT];

        printf("Введите ID студента: ");
        fgets(input, sizeof(input), stdin);
        student_id = atoi(input);

        printf("Введите название дисциплины: ");
        fgets(field_name, sizeof(field_name), stdin);
        field_name[strcspn(field_name, "\n")] = 0;

        printf("Введите новую оценку (2-5): ");
        fgets(input, sizeof(input), stdin);
        new_mark = atoi(input);

        update_grade(conn, session, student_id, field_name, new_mark);
        break;
      }

      case 4:
      {
        int student_id;
        char field_name[MAX_INPUT];

        printf("Введите ID студента: ");
        fgets(input, sizeof(input), stdin);
        student_id = atoi(input);

        printf("Введите название дисциплины: ");
        fgets(field_name, sizeof(field_name), stdin);
        field_name[strcspn(field_name, "\n")] = 0;

        delete_grade(conn, session, student_id, field_name);
        break;
      }

      case 5:
      {
        char last_name[MAX_INPUT];
        char first_name[MAX_INPUT];
        char group[MAX_INPUT];

        printf("Введите фамилию (можно частично): ");
        fgets(last_name, sizeof(last_name), stdin);
        last_name[strcspn(last_name, "\n")] = 0;

        printf("Введите имя (можно частично): ");
        fgets(first_name, sizeof(first_name), stdin);
        first_name[strcspn(first_name, "\n")] = 0;

        printf("Введите номер группы (можно частично): ");
        fgets(group, sizeof(group), stdin);
        group[strcspn(group, "\n")] = 0;

        search_student_grades(conn, session, last_name, first_name, group);
        break;
      }

      case 6:
      {
        char* query =
          "SELECT student_id, last_name, first_name, students_group_number "
          "FROM students LIMIT 10";
        execute_query_with_permission(conn, session, QUERY_SELECT, query, NULL);
        break;
      }

      case 7:
      {
        char* query =
          "SELECT field_id, field_name FROM fields ORDER BY field_name";
        execute_query_with_permission(conn, session, QUERY_SELECT, query, NULL);
        break;
      }

      case 8:
      {
        printf("\n%sТестирование защиты от SQL-инъекций%s\n",
               "========================================\n",
               "========================================\n");

        printf("1. Безопасный запрос:\n");
        char* safe_query = "SELECT * FROM students WHERE last_name = $1";
        QueryParams* params = create_string_param("Иванов");
        execute_query_with_permission(conn, session, QUERY_SELECT, safe_query,
                                      params);
        free_query_params(params);

        printf("\n2. Попытка SQL-инъекции:\n");
        char injection[] = "Иванов' OR '1'='1";
        printf("   Ввод: %s\n", injection);

        if (is_sql_injection(injection))
        {
          printf("   Обнаружена SQL-инъекция! Запрос заблокирован.\n");

          const int BUFFER_SIZE = 256;
          char escaped[BUFFER_SIZE];
          add_escape_quotes(injection, escaped, sizeof(escaped));
          printf("   Экранированный ввод: %s\n", escaped);
        }
        break;
      }

      case 0:
        printf("Выход из меню администратора...\n");
        break;

      default:
        printf("Неверный выбор!\n");
    }

    printf("\nНажмите Enter для продолжения...");
    getchar();

  } while (choice != 0);
}

void user_menu(Connection* conn, UserSession* session)
{
  int choice;
  char input[MAX_INPUT];

  do
  {
    printf("\n%sМеню пользователя%s\n",
           "========================================\n",
           "========================================\n");

    printf("1. Просмотреть мои оценки (если определен ID)\n");
    printf("2. Поиск оценок по ФИО и группе\n");
    printf("3. Попробовать SQL-инъекцию (демонстрация защиты)\n");
    printf("0. Выход\n");

    printf("\nВыберите действие: ");
    fgets(input, sizeof(input), stdin);
    choice = atoi(input);

    switch (choice)
    {
      case 1:
        if (session->student_id != -1)
        {
          get_student_grades(conn, session, session->student_id);
        }
        else
        {
          printf("ID студента не определен. Используйте поиск.\n");
        }
        break;

      case 2:
      {
        char last_name[MAX_INPUT];
        char first_name[MAX_INPUT];
        char group[MAX_INPUT];

        printf("Введите фамилию (можно частично): ");
        fgets(last_name, sizeof(last_name), stdin);
        last_name[strcspn(last_name, "\n")] = 0;

        printf("Введите имя (можно частично): ");
        fgets(first_name, sizeof(first_name), stdin);
        first_name[strcspn(first_name, "\n")] = 0;

        printf("Введите номер группы (можно частично): ");
        fgets(group, sizeof(group), stdin);
        group[strcspn(group, "\n")] = 0;

        search_student_grades(conn, session, last_name, first_name, group);
        break;
      }

      case 3:
      {
        printf("\n%sДемонстрация защиты от SQL-инъекций%s\n",
               "========================================\n",
               "========================================\n");

        printf("Попробуйте ввести SQL-инъекцию для поиска:\n");
        printf("Пример: Иванов' OR '1'='1\n");

        printf("\nВведите фамилию для поиска: ");
        char search_input[MAX_INPUT];
        fgets(search_input, sizeof(search_input), stdin);
        search_input[strcspn(search_input, "\n")] = 0;

        if (is_sql_injection(search_input))
        {
          printf("\n✓ Обнаружена попытка SQL-инъекции!\n");
          printf("  Ввод: %s\n", search_input);
          printf("\nЗапрос будет экранирован или заблокирован.\n");

          // Показываем, как бы это обрабатывалось
          const int BUFFER_SIZE = 512;
          char safe_query[BUFFER_SIZE];
          char escaped[BUFFER_SIZE];
          add_escape_quotes(search_input, escaped, sizeof(escaped));

          snprintf(safe_query, sizeof(safe_query),
                   "SELECT * FROM students WHERE last_name = '%s' LIMIT 5",
                   escaped);

          printf("\nЭкранированный запрос:\n%s\n", safe_query);
        }
        else
        {
          printf("\nБезопасный запрос будет выполнен.\n");
        }
        break;
      }

      case 0:
        printf("Выход из меню пользователя...\n");
        break;

      default:
        printf("Неверный выбор!\n");
    }

    printf("\nНажмите Enter для продолжения...");
    getchar();

  } while (choice != 0);
}

void junior_menu(Connection* conn, UserSession* session)
{
  int choice;
  char input[MAX_INPUT];

  do
  {
    printf("\n%sМеню младшего пользователя%s\n",
           "========================================\n",
           "========================================\n");

    printf("Только операции чтения:\n");
    printf("1. Поиск оценок по ФИО и группе\n");
    printf("2. Просмотреть всех студентов (первые 10)\n");
    printf("3. Просмотреть все дисциплины\n");
    printf("4. Попробовать изменить оценку (демонстрация отказа в доступе)\n");
    printf("0. Выход\n");

    printf("\nВыберите действие: ");
    fgets(input, sizeof(input), stdin);
    choice = atoi(input);

    switch (choice)
    {
      case 1:
      {
        char last_name[MAX_INPUT];
        char first_name[MAX_INPUT];
        char group[MAX_INPUT];

        printf("Введите фамилию (можно частично): ");
        fgets(last_name, sizeof(last_name), stdin);
        last_name[strcspn(last_name, "\n")] = 0;

        printf("Введите имя (можно частично): ");
        fgets(first_name, sizeof(first_name), stdin);
        first_name[strcspn(first_name, "\n")] = 0;

        printf("Введите номер группы (можно частично): ");
        fgets(group, sizeof(group), stdin);
        group[strcspn(group, "\n")] = 0;

        search_student_grades(conn, session, last_name, first_name, group);
        break;
      }

      case 2:
      {
        char* query =
          "SELECT student_id, last_name, first_name, students_group_number "
          "FROM students LIMIT 10";
        execute_query_with_permission(conn, session, QUERY_SELECT, query, NULL);
        break;
      }

      case 3:
      {
        char* query =
          "SELECT field_id, field_name FROM fields ORDER BY field_name";
        execute_query_with_permission(conn, session, QUERY_SELECT, query, NULL);
        break;
      }

      case 4:
      {
        printf("\nПопытка изменения оценки как junior пользователь:\n");
        printf("========================================\n");

        int result =
          update_grade(conn, session, 844688, "Операционные системы", 5);

        if (!result)
        {
          printf("\n✓ Как и ожидалось, доступ запрещен!\n");
          printf("  Junior пользователи могут только читать данные.\n");
        }
        break;
      }

      case 0:
        printf("Выход из меню...\n");
        break;

      default:
        printf("Неверный выбор!\n");
    }

    printf("\nНажмите Enter для продолжения...");
    getchar();

  } while (choice != 0);
}

int main()
{
  printf("Система управления оценками студентов с аутентификацией\n");
  printf("=====================================================\n\n");

  char* connection_parameters =
    "host=localhost "
    "port=5432 "
    "user=admin "
    "password=123456 "
    "dbname=enhanced_students";

  Connection* connection = connection_create(connection_parameters);
  if (connection == NULL)
  {
    printf("Ошибка выделения памяти для подключения\n");
    return 1;
  }

  set_log_level(LOG_INFO);

  UserSession* session = authenticate(connection);
  if (session == NULL)
  {
    printf("Не удалось аутентифицироваться. Завершение программы.\n");
    free(connection);
    return 1;
  }

  switch (session->role)
  {
    case USER_ROLE_ADMIN:
      admin_menu(connection, session);
      break;

    case USER_ROLE_JUNIOR:
      junior_menu(connection, session);
      break;

    case USER_ROLE_USER:
      user_menu(connection, session);
      break;

    default:
      printf("Неизвестная роль пользователя\n");
      break;
  }

  printf("\n%sЗавершение работы%s\n",
         "========================================\n",
         "========================================\n");

  printf("Статистика сессии:\n");
  printf("  Пользователь: %s\n", session->username);
  printf("  Роль: %s\n", get_user_role_string(session->role));
  printf("  Время работы: %ld секунд\n", time(NULL));

  destroy_user_session(session);
  finish_connection(connection);

  printf("\nПрограмма завершена успешно!\n");
  return 0;
}
