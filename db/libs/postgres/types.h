#ifndef POSTGRES_TYPES_H
#define POSTGRES_TYPES_H

typedef enum
{
  CONNECTION_STATUS_OK,
  CONNECTION_STATUS_BAD,
  CONNECTION_STATUS_CONNECTING
} ConnectionStatus;

typedef struct
{
  int count;
  const char** values;
  int* lengths;
  int* formats;
} QueryParams;

#endif  // POSTGRES_TYPES_H
