#include <stdio.h>

void viewPointer(void* p);
void printPointer(void* p);

int main()
{
    long long value = 0x1122334455667788;
    
    viewPointer(&value);
    printPointer(&value);
    
    char string[] = "0123456789abcdef";
    
    viewPointer(string);
    printPointer(string);
    
    return 0;
}

void viewPointer(void* p) {
    char *p1 = reinterpret_cast<char *>(p);
    unsigned short *p2 = reinterpret_cast<unsigned short *>(p);
    double *p3 = reinterpret_cast<double *>(p);
    
    if ((void *)p == (void *)p1 && (void *)p1 == (void *)p2
        && (void *)p2 == (void *)p3) {
        printf("Адреса указателей совпадают!\n");
    }
    
    printf("Следующие адреса каждого указателя:\np1 - %p\np2 - %p\np3 - %p\n", p1 + 1, p2 + 1, p3 + 1);
    printf("Разница между указателями p1: %ld Б\n", (p1 + 1 - p1) * sizeof(char));
    printf("Разница между указателями p2: %ld Б\n", (p2 + 1 - p2) * sizeof(unsigned short));
    printf("Разница между указателями p3: %ld Б\n", (p3 + 1 - p3) * sizeof(double));
}

void printPointer(void* p) {
    char *p1 = reinterpret_cast<char *>(p);
    unsigned short *p2 = reinterpret_cast<unsigned short *>(p);
    double *p3 = reinterpret_cast<double *>(p);
    
    printf("Значения:\np1 = %c\np2 = %hu\np3 = %lf\n", *p1, *p2, *p3);
}