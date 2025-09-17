#include <stdio.h>
#include <stdlib.h>

#define N 5
#define ALLOC(type) (type *)malloc(N * sizeof(type))
#define REPRESENTATION(type, array, N, format, text) {\
    printf("Массив типа %s %s\n", #type, text);\
    printf("[ ");\
    for (int i = 0; i < N; i++) {\
        printf(format, array[i]);\
        printf(" ");\
    }\
    printf("]\n\n");\
}

#define I 2
#define INPUT(type, array, format) {\
    printf("Для массива типа %s, введите значение элемента %d\n", #type, I + 1);\
    while ((scanf(format, array + I) != 1) || getchar() != '\n') {\
        puts("\nВы ввели значение неправильно! Пожалуйста, попробуйте еще раз...\n");\
        while (getchar() != '\n');\
    }\
}

int main()
{
    char* Mb = ALLOC(char);
    short* Ms = ALLOC(short);
    int* Ml = ALLOC(int);
    long long* Mq = ALLOC(long long);
    float* Mfs = ALLOC(float);
    double* Mfl = ALLOC(double);
    
    for (int i = 0; i < N; i++) {
        Mb[i] = 0xED;
        Ms[i] = 0xFADE;
        Ml[i] = 0xADE1A1DA;
        Mq[i] = 0xC1A551F1AB1E;
        Mfs[i] = - 2.0 / 7;
        Mfl[i] = - 2.0 / 7;
    }
    
    INPUT(char, Mb, "%1c");
    INPUT(short, Ms, "%6hd");
    INPUT(int, Ml, "%11d");
    INPUT(long long, Mq, "%20lld");
    INPUT(float, Mfs, "%f");
    INPUT(double, Mfl, "%lf");
    
    printf("\n-------------\n"); 
    REPRESENTATION(char, Mb, N, "%02X", "в шестнадцатеричном представлении");
    REPRESENTATION(char, Mb, N, "%b", "в двоичном представлении");
    REPRESENTATION(char, Mb, N, "%u", "в десятичном беззнаковом представлении");
    REPRESENTATION(char, Mb, N, "%d", "в десятичном знаковом представлении");
    REPRESENTATION(char, Mb, N, "%c", "в символьном представлении");
    
    printf("\n-------------\n"); 
    REPRESENTATION(short, Ms, N, "%04hX", "в шестнадцатеричном представлении");
    REPRESENTATION(short, Ms, N, "%hb", "в двоичном представлении");
    REPRESENTATION(short, Ms, N, "%hu", "в десятичном беззнаковом представлении");
    REPRESENTATION(short, Ms, N, "%hd", "в десятичном знаковом представлении");
    
    printf("\n-------------\n");
    REPRESENTATION(int, Ml, N, "%08X", "в шестнадцатеричном представлении");
    REPRESENTATION(int, Ml, N, "%b", "в двоичном представлении");
    REPRESENTATION(int, Ml, N, "%u", "в десятичном беззнаковом представлении");
    REPRESENTATION(int, Ml, N, "%d", "в десятичном знаковом представлении");
  
    printf("\n-------------\n"); 
    REPRESENTATION(long long, Mq, N, "%016llX", "в шестнадцатеричном представлении");
    REPRESENTATION(long long, Mq, N, "%llb", "в двоичном представлении");
    REPRESENTATION(long long, Mq, N, "%llu", "в десятичном беззнаковом представлении");
    REPRESENTATION(long long, Mq, N, "%lld", "в десятичном знаковом представлении");
    
    printf("\n-------------\n"); 
    REPRESENTATION(float, Mfs, N, "%A", "в шестнадцатеричном экспоненциальном представлении");
    REPRESENTATION(float, Mfs, N, "%e", "в десятичном экспоненциальном представлении");
    REPRESENTATION(float, Mfs, N, "%f", "в представлении с десятичной запятой");
    
    printf("\n-------------\n"); 
    REPRESENTATION(double, Mfl, N, "%2.5A", "в шестнадцатеричном экспоненциальном представлении");
    REPRESENTATION(double, Mfl, N, "%1.1e", "в десятичном экспоненциальном представлении");
    REPRESENTATION(double, Mfl, N, "%-2.10f", "в представлении с десятичной запятой");
    
    free(Mb);
    free(Ms);
    free(Ml);
    free(Mq);
    free(Mfs);
    free(Mfl);
    
    return 0;
}
