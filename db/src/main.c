#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void exit_with_error(PGconn *connection)
{
  PQfinish(connection);
  exit(1);
}

void print_query(PGconn *connection, char *query)
{
  PGresult *result = PQexec(connection, query);
  int rows = PQntuples(result);
  int columns = PQnfields(result);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    printf("No data retrieved!\n");
    PQclear(result);
    exit_with_error(connection);
  }

  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < columns; j++)
    {
      printf("%s ", PQgetvalue(result, i, j));
    }

    printf("\n");
  }

  PQclear(result);
}

void check_connection_status(PGconn *connection)
{
  printf("Connection status: ");
  switch (PQstatus(connection))
  {
    case CONNECTION_OK:
      printf("OK\n");
      break;
    case CONNECTION_BAD:
      printf("BAD\n");
      break;
    default:
      printf("OTHER (%d)\n", PQstatus(connection));
  }

  printf("Database: %s\n", PQdb(connection));
  printf("User: %s\n", PQuser(connection));
  printf("Host: %s\n", PQhost(connection));
  printf("Port: %s\n", PQport(connection));
}

int main()
{
  printf("Starting PostgreSQL connection...\n");

  PGconn *connection = PQconnectdb(
    "host=localhost "
    "port=5432 "
    "user=admin "
    "password=123456 "
    "dbname=enhanced_students");

  printf("Connection attempt completed.\n");
  check_connection_status(connection);

  if (PQstatus(connection) == CONNECTION_BAD)
  {
    int print_result = fprintf(stderr, "Connecton to database failed: %s\n",
                               PQerrorMessage(connection));
    exit_with_error(connection);
  }

  printf("Connected successfully! Testing query...\n");

  print_query(connection, "SELECT version()");
  print_query(connection, "SELECT * FROM students LIMIT 5;");

  printf("All operations complete!");
  PQfinish(connection);

  return 0;
}
