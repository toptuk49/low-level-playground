#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define N 8
#define INSERT_INDEX 2

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

void avxDoubleInsert(double* arr, size_t index, int32_t value);
void fillDouble(double* arr, size_t size, double value);

int main() {
    double* Mfl = ALLOC(double);

    fillDouble(Mfl, N, -2.0 / 7);
    REPRESENTATION(double, Mfl, N, "%lf", "исходный");

    avxDoubleInsert(Mfl, INSERT_INDEX, 8 / 3);
    REPRESENTATION(double, Mfl, N, "%lf", "после вставки");

    free(Mfl);

    return 0;
}

void fillDouble(double* arr, size_t size, double value) {
    size_t vector_size = size / 4;

    asm (
        "vbroadcastsd %[value], %%ymm0\n"
        "xorq %%rcx, %%rcx\n" // rcx = i
        "xorq %%rax, %%rax\n" // rax = сдвиг
        "double_loop_start:\n"
        "cmpq %[vector_size], %%rcx\n"
        "jge double_loop_end\n"

        "vmovupd %%ymm0, (%[arr_ptr], %%rax)\n"

        "addq $32, %%rax\n"
        "incq %%rcx\n"
        "jmp double_loop_start\n"
        "double_loop_end:\n"
        : // Выходные операнды
        : [arr_ptr] "r" (arr), [value] "m" (value), [vector_size] "r" (vector_size)
        : "ymm0", "rcx", "rax", "memory"
    );

    for (size_t i = vector_size * 4; i < size; ++i) {
        arr[i] = value;
    }
}

void avxDoubleInsert(double* arr, size_t index, int32_t value) {
    asm (
        "vcvtsi2sd %[value], %%xmm0, %%xmm0\n" // Конвертируем EAX в double, сохраняем в XMM0 и остальное берем из XMM0
        "movq %%xmm0, (%[base_arr], %[index], 8)"
        : // Выходные операнды
        : [value] "r" (value), [base_arr] "r" (arr), [index] "r" (index)
        : "memory", "xmm0"
    );
}
