#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#define INSERT_INDEX 7

void longLongArrayHexPrint(const int64_t* arr, int len) {
    for (int i = 0; i < len; ++i) {
        printf("%016" PRIx64 " ", arr[i]);
    }
    printf("\n");
}

int main() {
    int longLongLength = 8;
    int64_t* longLongArray = (int64_t*)malloc(longLongLength * sizeof(int64_t));
    if (longLongArray == NULL) {
        printf("Произошла ошибка при выделении памяти!");
        return 1;
    }

    for (int i = 0; i < longLongLength; ++i) {
        longLongArray[i] = 8 / 3;
    }

    printf("Иницилазированный массив long long: ");
    longLongArrayHexPrint(longLongArray, longLongLength);

    int index = INSERT_INDEX;

    asm (
        "movq %0, %%rax\n"
        "movslq %1, %%rdx\n"
        "movq $-1, (%%rax, %%rdx, 8)"
        : // Выходные операнды
        : "r" (longLongArray),
        "r" (index)
        : "memory", "%rax", "%rdx"
    );

    printf("Массив long long после вставки: ");
    longLongArrayHexPrint(longLongArray, longLongLength);

    free(longLongArray);

    return 0;
}