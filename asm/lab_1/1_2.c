#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#define FORMULA(type) "%s - %ld Б\n", #type, sizeof(type)

int main()
{
    printf(FORMULA(char));
    printf(FORMULA(char*));
    printf(FORMULA(bool));
    printf(FORMULA(wchar_t));
    printf(FORMULA(short));
    printf(FORMULA(unsigned short));
    printf(FORMULA(int));
    printf(FORMULA(long));
    printf(FORMULA(long long));
    printf(FORMULA(float));
    printf(FORMULA(double));
    printf(FORMULA(double*));
    printf(FORMULA(long double));
    printf(FORMULA(size_t));
    printf(FORMULA(ptrdiff_t));
    printf(FORMULA(void*));

    return 0;
}
