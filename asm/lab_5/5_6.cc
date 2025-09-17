#include <iostream>
#include <vector>

using namespace std;

void fillWithOddNumbersASM(int* array, int N) {
    asm (
        "xorl %%eax, %%eax\n" // i = 0
        "movl $1, %%edx\n" // odd = 1

        "print_loop:\n"
            "movl %%edx, (%%rdi, %%rax, 4)\n" // array[i] = odd
            "addl $2, %%edx\n"
            "incl %%eax\n"

            "cmpl %%eax, %%esi\n" // i < N
            "jl print_loop\n"
        : // Нет выходных операндов
        : "D" (array), "S" (N)
        : "memory", "%rax", "%rdx"
    );
}

void fillWithOddNumbersCPP(int* array, int N) {
    int odd = 1;
    for (int i = 0; i < N; ++i) {
        array[i] = odd;
        odd += 2;
    }
}

void printArray(int* array, int N) {
    for (int i = 0; i < N; ++i) {
        cout << array[i] << " ";
    }
    cout << endl;
}

int main() {
    int N;

    cout << "Введите N: ";
    cin >> N;

    int* array = new int[N];

    fillWithOddNumbersASM(array, N);
    cout << "Последовательность из нечетных чисел на ассемблере:" << endl;
    printArray(array, N);

    delete[] array;

    cout << "Последовательность из нечетных чисел без массива:" << endl;
    for (int i = 0; i < N; i++)
    {
        cout << 2 * i + 1 << " ";
    }
    cout << endl;

    return 0;
}