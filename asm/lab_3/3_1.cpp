#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#define INSERT_INDEX 2
/* ��������� ��� �-�� ������.
������������ ������������� ������� int16_t, int32_t � int64_t,
����� ������������� ���������� ������ �� ���� ����������.
������� PRIx16, PRIx32, PRIx64 ����� ��� ������ �������������
����� ������������� ��������. */

void shortArrayHexPrint(const int16_t* arr, int len) {
    for (int i = 0; i < len; ++i) {
        printf("%04" PRIx16 " ", arr[i]);
    }
    printf("\n");
}

void intArrayHexPrint(const int32_t* arr, int len) {
    for (int i = 0; i < len; ++i) {
        printf("%08" PRIx32 " ", arr[i]);
    }
    printf("\n");
}

void longLongArrayHexPrint(const int64_t* arr, int len) {
    for (int i = 0; i < len; ++i) {
        printf("%016" PRIx64 " ", arr[i]);
    }
    printf("\n");
}

int main() {
    int shortLength = 5;
    int16_t* shortArray = (int16_t*)malloc(shortLength * sizeof(int16_t));
    if (shortArray == NULL) {
        printf("��������� ������ ��� ��������� ������!");
        return 1;
    }

    // �������������� ������ (�������� �� ������ �� �1.�3)
    for (int i = 0; i < shortLength; ++i) {
        shortArray[i] = 8 / 3;
    }

    printf("����������������� ������ short: ");
    shortArrayHexPrint(shortArray, shortLength);

    asm (
        "movw $18, %0"
        : "=m" (shortArray[INSERT_INDEX])
    );

    printf("������ short ����� �������: ");
    shortArrayHexPrint(shortArray, shortLength);

    free(shortArray);

    printf("\n");

    int intLength = 5;
    int32_t* intArray = (int32_t*)malloc(intLength * sizeof(int32_t));
    if (intArray == NULL) {
        printf("��������� ������ ��� ��������� ������!");
        return 1;
    }

    for (int i = 0; i < intLength; ++i) {
        intArray[i] = 8 / 3;
    }

    printf("����������������� ������ int: ");
    intArrayHexPrint(intArray, intLength);

    asm (
        "movl $18, %0"
        : "=m" (intArray[INSERT_INDEX])
    );

    printf("������ int ����� �������: ");
    intArrayHexPrint(intArray, intLength);

    free(intArray);

    printf("\n");

    int longLongLength = 5;
    int64_t* longLongArray = (int64_t*)malloc(longLongLength * sizeof(int64_t));
    if (longLongArray == NULL) {
        printf("��������� ������ ��� ��������� ������!");
        return 1;
    }

    for (int i = 0; i < longLongLength; ++i) {
        longLongArray[i] = 8 / 3;
    }

    printf("����������������� ������ long long: ");
    longLongArrayHexPrint(longLongArray, longLongLength);

    asm (
        "movq $18, %0"
        : "=m" (longLongArray[INSERT_INDEX])
    );

    printf("������ long long ����� �������: ");
    longLongArrayHexPrint(longLongArray, longLongLength);

    free(longLongArray);

    return 0;
}
