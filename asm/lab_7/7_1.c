#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

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

void explore64BitNumber(void* p) {
   /* Из - за различий в размерах данных у GCC и MSVC, long занимает
   4 байта, вместо восьми, поэтому значение не умещается целиком
   в Windows.
   В GCC следует исправить long long на long */
   long long x = *(long long*)p;
   unsigned long long unsigned_x = *(unsigned long long*)p;

   printf("\nШестнадцатеричное представление: %16X", x);

   printf("\nДвоичное представление: ");
   for (int i = 63; i >= 0; i--) {
       printf("%d", (x & (1 << i)) ? 1 : 0);
   }

   printf("\nДесятичное беззнаковое представление: %llu", unsigned_x);

   printf("\nДесятичное знаковое представление: %+lld", x);

   printf("\nШестнадцатеричное экспоненциальное представление: %+3.3A", x);

   printf("\nДесятичное экспоненциальное представление: %+.2e", x);

   printf("\nПредставление с десятичной запятой: %+.2f", x);

   printf("\n");
}

float harmonic_forward_float(int N) {
    float sum = 0.0f;
    for (int i = 1; i <= N; i++) {
        sum += 1.0f / (float)i;
    }
    return sum;
}

float harmonic_reverse_float(int N) {
    float sum = 0.0f;
    for (int i = N; i >= 1; i--) {
        sum += 1.0f / (float)i;
    }
    return sum;
}

double harmonic_forward_double(int N) {
    double sum = 0.0;
    for (int i = 1; i <= N; i++) {
        sum += 1.0 / (double)i;
    }
    return sum;
}

double harmonic_reverse_double(int N) {
    double sum = 0.0;
    for (int i = N; i >= 1; i--) {
        sum += 1.0 / (double)i;
    }
    return sum;
}

int main() {
    int N_values[] = {1000, 1000000, 1000000000};
    
    printf("Оценки для float:\n");
    for (int i = 0; i < 3; i++) {
        int N = N_values[i];
        float sd = harmonic_forward_float(N);
        float sa = harmonic_reverse_float(N);
        
        printf("N=%.3e\n", (float)N);
        printf("  Sd(N)="); print32(&sd); printf("\n");
        printf("  Sa(N)="); print32(&sa); printf("\n");
        printf("  Difference: %.3e\n\n", fabsf(sd - sa));
    }
    
    printf("\nОценки для double:\n");
    for (int i = 0; i < 3; i++) {
        int N = N_values[i];
        double sd = harmonic_forward_double(N);
        double sa = harmonic_reverse_double(N);
        
        printf("N=%.3e\n", (double)N);
        printf("  Sd(N)="); print64(&sd); printf("\n");
        printf("  Sa(N)="); print64(&sa); printf("\n");
        printf("  Difference: %.3e\n\n", fabs(sd - sa));
    }
    
    return 0;
}