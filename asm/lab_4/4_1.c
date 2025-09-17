#include <stdio.h>

extern int f1(int, int);

int main() {
  int x = 4;
  int y = 6;

  int res = f1(x, y);
  printf("f1: %d\n", res);
  return 0;
}
