#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define N 8
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

void fillFloat(float* arr, size_t size, float value);
void fillDouble(double* arr, size_t size, double value);

int main() {
    float* Mfs = ALLOC(float);
    double* Mfl = ALLOC(double);

    fillFloat(Mfs, N, -2.0 / 7);
    fillDouble(Mfl, N, -2.0 / 7);

    REPRESENTATION(float, Mfs, N, "%f", "");
    REPRESENTATION(double, Mfl, N, "%lf", "");

    free(Mfs);
    free(Mfl);

    return 0;
}

void fillFloat(float* arr, size_t size, float value) {
    size_t vector_size = size / 8;

    asm(
        "vbroadcastss %[value], %%ymm0\n"
        "xorq %%rcx, %%rcx\n" // rcx = i
        "xorq %%rax, %%rax\n" // rax = сдвиг
        "float_loop_start:\n"
            "cmpq %[vector_size], %%rcx\n"
            "jge float_loop_end\n"

            "vmovups %%ymm0, (%[arr_ptr], %%rax)\n"

            "addq $32, %%rax\n"
            "incq %%rcx\n"
            "jmp float_loop_start\n"
        "float_loop_end:\n"
        : // Выходные операнды
        : [arr_ptr] "r" (arr), [value] "m" (value), [vector_size] "r" (vector_size)
        : "ymm0", "rcx", "rax", "memory"
    );

    for (size_t i = vector_size * 8; i < size; ++i) {
        arr[i] = value;
    }
}

void fillDouble(double* arr, size_t size, double value) {
    size_t vector_size = size / 4;

    asm(
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
