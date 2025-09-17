#include <iostream>
#include <cstdio> // Для printf

using namespace std;

int main() {
    int N;

    cout << "Введите N: ";
    cin >> N;

    cout << "Последовательность из нечетных чисел на ассемблере:" << endl;

    asm (
        "xorl %%rax, %%rax\n" // i = 0
        "movl $1, %%rdx\n" // odd = 1

        "loop_start:\n"
            // Подготовить аргументы для printf
            "movq $format_string, %%rdi\n" // Первый аргумент: форматная строка
            "movl %%edx, %%esi\n" // Второй аргумент: нечетное число

            "pushq %%rax\n"
            "pushq %%rdi\n"
            "pushq %%rsi\n"
            
            "call printf\n"

            "popq %%rsi\n"
            "popq %%rdi\n"
            "popq %%rax\n"

            "addl $2, %%edx\n" // odd += 2
            "incl %%eax\n" // i++
            "cmpl %%eax, %1\n" // Сравнить i и N
        "jl loop_start\n"

        "movq $newline_string, %%rdi\n"
        "call puts\n"
        : // Нет выходных операндов
        : "r" (N)
        : "memory", "%rax", "%rdx", "%rdi", "%rsi"
    );
    return 0;
}

extern "C" {
  int printf(const char *format, ...);
  int puts(const char *s);
}

namespace {
    const char format_string[] = "%d ";
    const char newline_string[] = "\n";
}