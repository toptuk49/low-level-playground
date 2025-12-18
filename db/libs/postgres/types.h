#ifndef POSTGRES_TYPES_H
#define POSTGRES_TYPES_H

typedef enum
{
  CONNECTION_STATUS_OK,
  CONNECTION_STATUS_BAD,
  CONNECTION_STATUS_CONNECTING
} ConnectionStatus;

typedef enum
{
  QUERY_SELECT,
  QUERY_UPDATE,
  QUERY_INSERT,
  QUERY_DELETE,
  QUERY_OTHER
} QueryType;

typedef enum
{
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_SECURITY
} LogLevel;

typedef struct
{
  int count;
  const char** values;
  int* lengths;
  int* formats;
} QueryParams;

#endif  // POSTGRES_TYPES_H
