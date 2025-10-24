#ifndef POSTGRES_POSTGRES_H
#define POSTGRES_POSTGRES_H

#include "types.h"

typedef struct Connection Connection;

Connection* connection_create(char* parameters);
void print_connection_status(Connection* connection);
ConnectionStatus get_connection_status(Connection* connection);
const char* get_connection_status_string(ConnectionStatus status);
char* get_error_message(Connection* connection);
void exit_with_error(Connection* connection);
void finish_connection(Connection* connection);

// Unsafe request
void print_query(Connection* connection, char* query);

// Safe request
QueryParams* create_query_params(int count);
void add_string_param(QueryParams* params, int index, const char* value);
void add_int_param(QueryParams* params, int index, int value);
void free_query_params(QueryParams* params);
void print_query_params(Connection* connection, char* query,
                        QueryParams* params);
// Safe UPDATE/INSERT/DELETE execution
int execute_update_params(Connection* connection, char* query,
                          QueryParams* params);

#endif  // LIBS_POSTGRES_H
