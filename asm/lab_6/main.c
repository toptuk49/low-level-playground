#include <stdint.h>
#include <stdio.h>

extern int32_t calculateZ(int32_t x, int32_t y);
extern void roundToAMultipleofDInAssembly(void *p);
extern void performBitwiseOperations(void *p);
extern int calculateSum(void *p, size_t N);
extern void divComputation(int32_t x, int32_t y, 
   uint32_t* z_unsigned, uint32_t* w_unsigned,
   int32_t* z_signed, int32_t* w_signed);

void printResults(int32_t x, int32_t y, uint32_t zu, uint32_t wu, int32_t zs, int32_t ws) {
    printf("x = %d, y = %d | ", x, y);
    printf("Знаковое: %u / (%u - 3) = %u, Остаток %u | ", 
            (uint32_t)x, (uint32_t)y, zu, wu);
    printf("Беззнаковое: %+d/(%+d-3) = %+d, Остаток%+d\n", 
            x, y, zs, ws);
}

void explore16BitNumber(void* p) {
    short x = *(short*)p;
    unsigned short unsigned_x = *(unsigned short*)p;

    printf("\nШестнадцатеричное представление: %04X", x);

    printf("\nДвоичное представление: ");
    for (int i = 15; i >= 0; i--) {
        printf("%d", (x & (1 << i)) ? 1 : 0);
    }

    printf("\nДесятичное беззнаковое представление: %u", unsigned_x);

    printf("\nДесятичное знаковое представление: %d", x);

    printf("\n");
}

void roundToAMultipleofDInCLanguage(void *p) { // fc16_c
    uint16_t x = *(uint16_t*)p;
    const uint16_t D = 16;
    const uint16_t mask = D - 1;
    
    printf("Реализация на языке C:\n");
    printf("Исходное число: ");
    explore16BitNumber(x);
    
    uint16_t x1 = x & ~mask;
    printf("Число, округленное вниз: ");
    explore16BitNumber(x1);
    
    uint16_t x2 = (x + mask) & ~mask;
    printf("Число, округленное вверх: ");
    explore16BitNumber(x2);
}

int32_t calculateSecondZ(int32_t x) {
    int32_t z;

    asm (
        "lea %rax, [%rdi*4 + %rdi + 5]\n" // rax = x*4 + x + 5 = 5x + 5
        "mov %0, %rax\n"
        : "=r" (z)
        : "D" (x)
        : "%rax"
    );

    return z;
}

void firstTask(void){
    uint16_t nums[] = {0, 1, 31, 32, 33, 255};
    size_t N = sizeof(nums) / sizeof(uint16_t);

    printf("Беззнаковое округление до значения, кратного D.\n");
    for (size_t i = 0; i < N; i++) {
        printf("Исходное значение = %u\n", nums[i]);
        roundToAMultipleofDInCLanguage(&nums[i]);
        roundToAMultipleofDInAssembly(&nums[i]);
    }
}

void secondTask(void){
    short nums[] = {37, -33};
    for (int i = 0; i < 2; i++) {
        printf("\nTest case %d: value = %d\n", i+1, nums[i]);
        performBitwiseOperations(&nums[i]);
    }
}

void thirdTask(void){
    int32_t x = 5;
    int32_t y = 3;
    int32_t z = calculateZ(x, y);

    printf("z = %d\n", z);
}

void fourthTask(void){
    int32_t x1 = -3;
    int32_t y1 = 2;
    uint32_t zu, wu;
    int32_t zs, ws;
    
    divComputation(x1, y1, &zu, &wu, &zs, &ws);
    printResults(x1, y1, zu, wu, zs, ws);
}

void fifthTask(void){
    int32_t nums[] = {0, 1, 5};
    size_t N = sizeof(nums) / sizeof(int32_t);
    
    for (size_t i = 0; i < N; i++) {
        printf("x = %d, z = %d\n", nums[i], calculateSecondZ(nums[i]));
    }
}

void sixthTask(void){
    int nums[] = {2, 3, 5};
    size_t N = sizeof(nums) / sizeof(int);
    
    int sum = calculateSum(nums, N);
    printf("Результат суммы: %d\n", sum);
}

int main() {
    printf("Задание 1:\n");
    firstTask();

    printf("Задание 2:\n");
    secondTask();

    printf("Задание 3:\n");
    thirdTask();

    printf("Задание 4:\n");
    fourthTask();

    printf("Задание 5:\n");
    fifthTask();

    printf("Задание 6:\n");
    sixthTask();

    return 0;
}