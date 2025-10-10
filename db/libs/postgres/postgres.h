#ifndef POSTGRES_POSTGRES_H
#define POSTGRES_POSTGRES_H

#include "types.h"

typedef struct Connection Connection;

Connection *connection_create(char *parameters);
void print_connection_status(Connection *connection);
void print_query(Connection *connection, char *query);
ConnectionStatus get_connection_status(Connection *connection);
char *get_error_message(Connection *connection);
void exit_with_error(Connection *connection);
void finish_connection(Connection *connection);

#endif  // LIBS_POSTGRES_H
