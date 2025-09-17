#include <stdio.h>
#include <stdint.h>

int main() {
    int64_t x = 10, y = 5, z = 0, w = 0;

    printf("Значения x, y, z, w соответственно: %ld, %ld, %ld, %ld\n", x, y, z, w);

    asm(
        "movq %[x], %%rax\n"
        "movq %[y], %%rbx\n"
        "addq %%rbx, %%rax\n"
        "movq %%rax, %[z]\n"

        "movq %[x], %%rax\n"
        "subq %%rbx, %%rax\n"
        "movq %%rax, %[w]\n"
        : [z] "=m" (z), [w] "=m" (w)
        : [x] "r" (x), [y] "r" (y)
        : "memory", "rax", "rbx"
    );

    printf("Значения x, y, z, w после вставки: %ld, %ld, %ld, %ld\n", x, y, z, w);

    return 0;
}