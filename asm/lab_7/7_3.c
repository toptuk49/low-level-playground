#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

extern void inc32_asm(void *p);

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

int main() {
    float a = 1.0f;
    float b = 2.0f;
    float c = 12345678.0f;
    float d = 123456789.0f;
    
    printf("Исходные значения:\n");
    printf("\na:\n"); print32(&a);
    printf("\nb:\n"); print32(&b);
    printf("\nc:\n"); print32(&c);
    printf("\nd:\n"); print32(&d);
    
    inc32_asm(&a);
    inc32_asm(&b);
    inc32_asm(&c);
    inc32_asm(&d);
    
    printf("\nПосле инкрементирования:\n");
    printf("\na:\n"); print32(&a);
    printf("\nb:\n"); print32(&b);
    printf("\nc:\n"); print32(&c);
    printf("\nd:\n"); print32(&d);
    
    return 0;
}