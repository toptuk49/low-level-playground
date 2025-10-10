#include "postgres.h"

#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

struct Connection
{
  PGconn *connection;
};

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

static const char *pq_status_to_string(ConnStatusType pq_status)
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

Connection *connection_create(char *parameters)
{
  Connection *connection = (Connection *)malloc(sizeof(Connection));
  if (connection == NULL)
  {
    printf("Error creating Connection object!");
    return NULL;
  }

  connection->connection = PQconnectdb(parameters);
  return connection;
}

void print_connection_status(Connection *connection)
{
  ConnStatusType pq_status = PQstatus(connection->connection);
  ConnectionStatus our_status = pq_status_to_connection_status(pq_status);

  printf("Connection status: %s (our: %s)\n", pq_status_to_string(pq_status),
         get_connection_status_string(our_status));

  printf("Database: %s\n", PQdb(connection->connection));
  printf("User: %s\n", PQuser(connection->connection));
  printf("Host: %s\n", PQhost(connection->connection));
  printf("Port: %s\n", PQport(connection->connection));
}

void print_query(Connection *connection, char *query)
{
  PGresult *result = PQexec(connection->connection, query);

  if (PQresultStatus(result) != PGRES_TUPLES_OK)
  {
    printf("No data retrieved! Error: %s\n",
           PQerrorMessage(connection->connection));
    PQclear(result);
    exit_with_error(connection);
  }

  int rows = PQntuples(result);
  int columns = PQnfields(result);

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
  ConnStatusType pq_status = PQstatus(connection->connection);
  return pq_status_to_connection_status(pq_status);
}

const char *get_connection_status_string(ConnectionStatus status)
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
  if (connection && connection->connection)
  {
    PQfinish(connection->connection);
    connection->connection = NULL;
  }

  free(connection);
}
