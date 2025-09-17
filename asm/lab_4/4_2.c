#include <stdio.h>

extern double f2(double, double);
int main() {
  double x = 5.5;
  double y = 2.0;

  double res = f2(x, y);
  printf("f2: %lf\n", res);

  return 0;
}
