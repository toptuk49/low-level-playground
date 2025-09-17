#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0

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

#define K 10

void clearStdin();
void fillBuffer(char* buffer);

int main()
{
    char buffer[K];
    
    fillBuffer(buffer);
    printf("Введите строку s1: ");
    scanf("%s", buffer);
    char* s1 = (char*)malloc(strlen(buffer) + 1);
    strcpy(s1, buffer);

    fillBuffer(buffer);
    char* s2;
    while (true) {
        printf("\nВведите строку s2: ");
        scanf("%10s", buffer);
        
        char c;
        if ((c = getchar()) == '\n' || c == EOF) {
            s2 = (char*)malloc(strlen(buffer) + 1);
            strcpy(s2, buffer);
            break;
        }
         
        printf("\nПроизошло переполнение буфера! Пожалуйста, введите строку меньшей длины еще раз...");
        clearStdin();
        fillBuffer(buffer);
        
    }

    fillBuffer(buffer);
    printf("Введите строку s3: ");
    scanf("%[^\n]", buffer);
    char* s3 = (char*)malloc(strlen(buffer) + 1);
    strcpy(s3, buffer);
    
    printf("***%s***\n***%s***\n***%s***", s1, s2, s3);
    
    free(s1);
    free(s2);
    free(s3);
    
    return 0;
}

void clearStdin() {
    char c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

void fillBuffer(char* buffer) {
    for (int i = 0; i < K; i++) {
        buffer[i] = '\0';
    }
}
