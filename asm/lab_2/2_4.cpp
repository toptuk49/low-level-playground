#include <stdio.h>

#include <limits>

#define MIN_LIMIT(type) (long long)std::numeric_limits<type>::min()
#define MAX_LIMIT(type) (unsigned long long)std::numeric_limits<type>::max()

int main() {
   printf("8-bit char: min = %lld, max = %lld\n", MIN_LIMIT(char), MAX_LIMIT(char));
   printf("8-bit unsigned char: min = %lld, max = %lld\n", MIN_LIMIT(unsigned char), MAX_LIMIT(unsigned char));

   printf("16-bit short: min = %lld, max = %lld\n", MIN_LIMIT(short), MAX_LIMIT(short));
   printf("16-bit unsigned short: min = %lld, max = %lld\n", MIN_LIMIT(unsigned short), MAX_LIMIT(unsigned short));

   printf("32-bit int: min = %lld, max = %lld\n", MIN_LIMIT(int), MAX_LIMIT(int));
   printf("32-bit unsigned int: min = %lld, max = %lld\n", MIN_LIMIT(unsigned int), MAX_LIMIT(unsigned int));

   printf("64-bit long long: min = %lld, max = %lld\n", MIN_LIMIT(long long), MAX_LIMIT(long long));
   printf("64-bit unsigned long long: min = %lld, max = %llu\n", MIN_LIMIT(unsigned long long), MAX_LIMIT(unsigned long long));

   // 2^N значений может принимать переменная беззнакового 𝑁-битного типа
   // 2^N значений может принимать переменная знакового 𝑁-битного типа
   /* Это связано со значением N таким образом, что у одного бита может быть два значения,
   а всего значений можно выбрать 2^N штук по правилу произведения */

   printf("32-bit float: min = %f, max = %f\n", std::numeric_limits<float>::min(), std::numeric_limits<float>::max());
   printf("64-bit double: min = %lf, max = %lf\n", std::numeric_limits<double>::min(), std::numeric_limits<double>::max());

   return 0;
}

