#include <stdio.h>

int print_eight_params(int a, int b, int c, int d, int e, int f, int g, int h) {
    printf("Параметр 1: %d\n", a);
    printf("Параметр 2: %d\n", b);
    printf("Параметр 3: %d\n", c);
    printf("Параметр 4: %d\n", d);
    printf("Параметр 5: %d\n", e);
    printf("Параметр 6: %d\n", f);
    printf("Параметр 7: %d\n", g);
    printf("Параметр 8: %d\n", h);
    
    return h;
}