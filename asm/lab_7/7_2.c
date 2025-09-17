#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>  // Для memcpy

void explore32BitNumber(void* p) {
   int x = *(int*)p;
   unsigned unsigned_x = *(unsigned*)p;

   printf("\nШестнадцатеричное представление: %08X", x);

   printf("\nДвоичное представление: ");
   for (int i = 31; i >= 0; i--) {
       printf("%d", (x & (1 << i)) ? 1 : 0);
   }

   printf("\nДесятичное беззнаковое представление: %u", unsigned_x);

   printf("\nДесятичное знаковое представление: %+d", x);

   printf("\nШестнадцатеричное экспоненциальное представление: %+3.3A", x);

   printf("\nДесятичное экспоненциальное представление: %.2e", x);

   printf("\nПредставление с десятичной запятой: %+.2f", x);

   printf("\n");
}

float absolute_value(float x) {
    uint32_t bits;
    memcpy(&bits, &x, sizeof(bits));
    bits &= 0x7FFFFFFF; // Сбрасываем знаковый бит (31-й)
    memcpy(&x, &bits, sizeof(bits));
    return x;
}

int main() {
    float x = -3.14f;
    printf("Исходное значение:\n");
    print32(&x);
    
    float abs_x = absolute_value(x);
    printf("\nМодуль значения:\n");
    print32(&abs_x);
    
    return 0;
}