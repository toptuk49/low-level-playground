#include <stdio.h>
#include <stdlib.h>

#define N 5
#define ALLOC(type, c) (type *)malloc(c * sizeof(type))
#define REPRESENTATION(type, array, N, format, text) {\
    printf("Массив типа %s %s\n", #type, text);\
    printf("[ ");\
    for (int i = 0; i < N; i++) {\
        printf(format, array[i]);\
        printf(" ");\
    }\
    printf("]\n\n");\
}

int main()
{
    int* Ml = ALLOC(int, N);
    
    for (int i = 0; i < N; i++) {
        Ml[i] = 0xADE1A1DA;
    }
    
    printf("\n-------------\n");
    REPRESENTATION(int, Ml, N, "%08X", "в шестнадцатеричном представлении");
    REPRESENTATION(int, Ml, N, "%b", "в двоичном представлении");
    REPRESENTATION(int, Ml, N, "%u", "в десятичном беззнаковом представлении");
    REPRESENTATION(int, Ml, N, "%d", "в десятичном знаковом представлении");
    
    printf("Для массива типа int, введите значение 5 элементов:\n");
    int count;
    while ((count = scanf("%10d %10d %10d %10d %10d", Ml, Ml+1, Ml+2, Ml+3, Ml+4)) != 5) {
        printf("\nВы ввели %d переменных! Пожалуйста, попробуйте еще раз...\n", count);
    }
    printf("Все круто! Вы ввели 5 переменных!\n");

    printf("\n-------------\n");
    REPRESENTATION(int, Ml, N, "%08X", "в шестнадцатеричном представлении");
    REPRESENTATION(int, Ml, N, "%b", "в двоичном представлении");
    REPRESENTATION(int, Ml, N, "%u", "в десятичном беззнаковом представлении");
    REPRESENTATION(int, Ml, N, "%d", "в десятичном знаковом представлении");
    
    free(Ml);
    
    return 0;
}
