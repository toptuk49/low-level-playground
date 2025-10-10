#include <stdio.h>

#include "postgres.h"
#include "types.h"

int main()
{
  printf("Starting PostgreSQL connection...\n");

  char* connectionParameters =
    "host=localhost "
    "port=5432 "
    "user=admin "
    "password=123456 "
    "dbname=enhanced_students";

  Connection* connection = connection_create(connectionParameters);

  printf("Connection attempt completed.\n");
  print_connection_status(connection);

  if (get_connection_status(connection) == CONNECTION_STATUS_BAD)
  {
    int print_result = fprintf(stderr, "Connecton to database failed: %s\n",
                               get_error_message(connection));
    exit_with_error(connection);
  }

  printf("Connected successfully! Testing query...\n");

  print_query(connection, "SELECT version()");
  print_query(connection, "SELECT * FROM students LIMIT 5;");

  printf("All operations complete!");
  finish_connection(connection);

  return 0;
}
