#include <stdint.h>
#include <stdio.h>

void cpuid(int func, int* eax, int* ebx, int* ecx, int* edx) {
    asm volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(func)
    );
}

int main() {
    printf("Проверка поддержки инструкций AVX и SSE:\n");

    int eax, ebx, ecx, edx;

    // Проверка поддержки SSE и SSE2
    cpuid(1, &eax, &ebx, &ecx, &edx);

    // edx[25] - SSE
    // edx[26] - SSE2
    // ecx[0]  - SSE3
    // ecx[9]  - SSSE3
    // ecx[19] - SSE4.1
    // ecx[20] - SSE4.2

    printf("SSE:   %s\n", (edx & (1 << 25)) ? "Yes" : "No");
    printf("SSE2:  %s\n", (edx & (1 << 26)) ? "Yes" : "No");
    printf("SSE3:  %s\n", (ecx & (1 << 0)) ? "Yes" : "No");
    printf("SSSE3: %s\n", (ecx & (1 << 9)) ? "Yes" : "No");
    printf("SSE4.1: %s\n", (ecx & (1 << 19)) ? "Yes" : "No");
    printf("SSE4.2: %s\n", (ecx & (1 << 20)) ? "Yes" : "No");

    // Проверка поддержки AVX
    cpuid(1, &eax, &ebx, &ecx, &edx);

    // ecx[28] - AVX
    if (ecx & (1 << 28)) {
        // Проверяем расширенные характеристики AVX
        cpuid(7, &eax, &ebx, &ecx, &edx);

        // ebx[5] - AVX2
        printf("AVX:   Yes\n");
        printf("AVX2:  %s\n", (ebx & (1 << 5)) ? "Yes" : "No");

        // Проверка поддержки AVX-512
        cpuid(7, &eax, &ebx, &ecx, &edx);

        printf("AVX-512F:  %s\n",
            (ebx & (1 << 16)) ? "Yes" : "No");  // Foundation
        printf("AVX-512DQ:  %s\n",
            (ebx & (1 << 17)) ? "Yes" : "No");  // Double & Quad word
        printf("AVX-512IFMA: %s\n",
            (ebx & (1 << 21)) ? "Yes" : "No");  // Integer Fused Multiply Add
        printf("AVX-512PF:  %s\n",
            (ebx & (1 << 26)) ? "Yes" : "No");  // Prefetch
        printf("AVX-512ER:  %s\n",
            (ebx & (1 << 27)) ? "Yes" : "No");  // Exponential & Reciprocal
        printf("AVX-512CD:  %s\n",
            (ebx & (1 << 28)) ? "Yes" : "No");  // Conflict Detection

    }
    else {
        printf("AVX:   No\n");
        printf("AVX2:  No\n");
        printf("AVX-512F: No\n");
        printf("AVX-512DQ: No\n");
        printf("AVX-512IFMA: No\n");
        printf("AVX-512PF: No\n");
        printf("AVX-512ER: No\n");
        printf("AVX-512CD: No\n");
    }

    return 0;
}