#ifndef POSTGRES_POSTGRES_H
#define POSTGRES_POSTGRES_H

#include <stdbool.h>
#include <stdlib.h>

#include "types.h"

typedef struct Connection Connection;

// Основные функции подключения
Connection* connection_create(char* parameters);
void print_connection_status(Connection* connection);
ConnectionStatus get_connection_status(Connection* connection);
const char* get_connection_status_string(ConnectionStatus status);
char* get_error_message(Connection* connection);
void exit_with_error(Connection* connection);
void finish_connection(Connection* connection);

// Логирование
void set_log_level(LogLevel level);
void postgres_log(LogLevel level, const char* format, ...);
void log_sql_injection_attempt(const char* query, const char* user_input);

// Методы защиты от SQL-инъекций (из теории)
void correct_apostrof(char* query);
void add_escape_quotes(const char* query, char* query_new, size_t buffer_size);
bool is_sql_injection(const char* input);

// Фабричные методы для QueryParams
QueryParams* create_string_param(const char* value);
QueryParams* create_int_param(int value);
QueryParams* create_query_params(int count);

// Методы работы с параметрами
void add_string_param(QueryParams* params, int index, const char* value);
void add_int_param(QueryParams* params, int index, int value);
void free_query_params(QueryParams* params);

// Единый интерфейс для запросов (CRUD)
int execute_query(Connection* connection, QueryType type, const char* query,
                  QueryParams* params);
void print_query(Connection* connection,
                 const char* query);  // Небезопасный метод
void print_query_params(Connection* connection, const char* query,
                        QueryParams* params);  // Безопасный метод

// Обработка транзакций
int begin_transaction(Connection* connection);
int commit_transaction(Connection* connection);
int rollback_transaction(Connection* connection);
int execute_in_transaction(Connection* connection, const char* query,
                           QueryParams* params);

// Подготовленные выражения (оптимизация)
typedef struct PreparedStatement PreparedStatement;
PreparedStatement* prepare_statement(Connection* connection, const char* name,
                                     const char* query);
int execute_prepared(Connection* connection, const char* name,
                     QueryParams* params);
void free_prepared_statement(PreparedStatement* statement);

// Представления
int create_view(Connection* connection, const char* view_name,
                const char* query);
int drop_view(Connection* connection, const char* view_name);
int materialize_view(Connection* connection, const char* view_name);

#endif  // POSTGRES_POSTGRES_H
