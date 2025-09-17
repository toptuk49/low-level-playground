#include <stdio.h>
#include <stdlib.h>

#define N 5
#define R 4
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
  
    printf("Адрес начала массива Ml - %p\n", Ml);
    printf("Адрес 1-го элемента массива Ml - %p\n", &(Ml[0]));
    printf("Адрес 2-го элемента массива Ml - %p\n", &(Ml[1]));
    
    printf("Размер элемента массива = %ld, размер адреса на элемент = %ld\n\n", sizeof(Ml[0]), sizeof(&(Ml[0])));
    
    
    
    int** MM = ALLOC(int*, N);
    for (int i = 0; i < N; i++)
        MM[i] = ALLOC(int, R);
        
        
    printf("Адрес MM[0][0] - %p\n", &(MM[0][0]));
    printf("Адрес MM[0][1] - %p\n", &(MM[0][1]));
    printf("Адрес MM[1][0] - %p\n", &(MM[1][0]));
    printf("Адрес MM[1][1] - %p\n", &(MM[1][1]));
    
    free(Ml);
    for (int i = 0; i < N; i++)
        free(MM[i]);
    free(MM);
    
    return 0;
}
