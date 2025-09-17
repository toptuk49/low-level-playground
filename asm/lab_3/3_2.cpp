#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#define INSERT_INDEX 2

void intArrayHexPrint(const int32_t* arr, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        printf("%08" PRIx32 " ", arr[i]);
    }
    printf("\n");
}

int main() {
    size_t arrayLength = 5;
    size_t index = INSERT_INDEX;
    int32_t* array = (int32_t*)malloc(arrayLength * sizeof(int32_t));
    if (array == NULL) {
        printf("��������� ������ ��� ��������� ������!");
        return 1;
    }

    for (size_t i = 0; i < arrayLength; ++i) {
        array[i] = 8 / 3;
    }
    int32_t* baseAddress = array;

    printf("������������������ ������ Ml � ������� ���� int: ");
    intArrayHexPrint(array, arrayLength);

    asm (
        "movl $-1, (%0, %1, 2)"
        : /* ��� �������� ��������� */
        : "r" (baseAddress),
        "r" (index)
        : "memory"
    );

    printf("������ Ml � ������� ���� int ����� �������:  ");
    intArrayHexPrint(array, arrayLength);

    free(array);

    return 0;
}