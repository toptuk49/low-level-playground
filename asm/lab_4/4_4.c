#include <stdio.h>

int main() {
    unsigned short i16;
    unsigned int i32;
    unsigned long long i64;
    float f32;
    double f64;

    scanf("%hu %u %llu %f %lf", &i16, &i32, &i64, &f32, &f64);

    printf("i16: %hu, i32: %u, i64: %llu, f32: %f, f64: %lf\n", i16, i32, i64, f32, f64);
    
    return 0;
}