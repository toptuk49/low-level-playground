#include <string.h>
#include <stdlib.h>

#include "revert_string.h"

#define BUFFER_LENGTH 1024

void FillBuffer(char* buffer);

void RevertString(char *str)
{
  int length = strlen(str);
  char buffer[BUFFER_LENGTH];
  FillBuffer(buffer);
  
  int j = 0;
  for (int i = length - 1; i >= 0; i--) {
    buffer[j] = str[i];
    j++;
  }

  for (int i = 0; i < length; i++) {
    str[i] = buffer[i];
  }
}

void FillBuffer(char* buffer) {
  for (int i = 0; i < BUFFER_LENGTH; i++) {
    buffer[i] = '0';
  }
}

