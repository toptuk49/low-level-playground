#include "postgres.h"

#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

struct Connection
{
  PGconn *connection;
};

Connection *connection_create(char *parameters)
{
  Connection *connection = (Connection *)malloc(sizeof(Connection));
  if (connection == NULL)
  {
    printf("Произошла ошибка при создании объекта Connection!");
  }

  connection->connection = PQconnectdb(parameters);
  return connection;
}

void print_connection_status(Connection *connection)
{
  printf("Connection status: ");
  switch (PQstatus(connection->connection))
  {
    case CONNECTION_OK:
      printf("OK\n");
      break;
    case CONNECTION_BAD:
      printf("BAD\n");
      break;
    default:
      printf("OTHER (%d)\n", PQstatus(connection->connection));
  }

  printf("Database: %s\n", PQdb(connection->connection));
  printf("User: %s\n", PQuser(connection->connection));
  printf("Host: %s\n", PQhost(connection->connection));
  printf("Port: %s\n", PQport(connection->connection));
}

void print_query(Connection *connection, char *query)
{
  PGresult *result = PQexec(connection->connection, query);
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

ConnectionStatus get_connection_status(Connection *connection)
{
  ConnStatusType status = PQstatus(connection->connection);
  return (status == CONNECTION_OK) ? OK : BAD;
}

char *get_error_message(Connection *connection)
{
  return PQerrorMessage(connection->connection);
}

void exit_with_error(Connection *connection)
{
  finish_connection(connection);
  exit(1);
}

void finish_connection(Connection *connection)
{
  PQfinish(connection->connection);
}
