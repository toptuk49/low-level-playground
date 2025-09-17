#include <stdio.h>
#include <string.h>

#include <cwchar>

void memoryDumpResearch(void* locationPointer, size_t memorySizeInBytes);

int main()
{
    printf("В этой программе мы исследуем участок памяти определенного размера.\n");
    printf("Делаем мы это, выводя участок памяти в символьном виде, что позволяет\n");
    printf("исследовать, как хранятся в памяти значения определенного типа.\n");
    printf("---------------------------------\n");
    
    int x = 9;
    printf("\nКак хранится в памяти значение %d типа int:\n", x);
    memoryDumpResearch(&x, sizeof(int));
    
    float xFloat = 9;
    printf("\nКак хранится в памяти значение %f типа float:\n", xFloat);
    memoryDumpResearch(&xFloat, sizeof(float));
    
    char engString[] = "abcdef";
    char rusString[] = "абвгде";
    printf("\nКак хранится в памяти значение %s типа char[]:\n", engString);
    memoryDumpResearch(engString, sizeof(char) * strlen(engString));
    printf("\nКак хранится в памяти значение %s типа char[]:\n", rusString);
    memoryDumpResearch(rusString, sizeof(char) * strlen(engString));
    
    wchar_t wideEngString[] = L"abcdef";
    wchar_t wideRusString[] = L"абвгде";
    printf("\nКак хранится в памяти значение %ls типа wchar_t[]:\n", wideEngString);
    memoryDumpResearch(wideEngString, sizeof(wchar_t) * std::wcslen(wideEngString));
    printf("\nКак хранится в памяти значение %ls типа wchar_t[]:\n", wideRusString);
    memoryDumpResearch(wideRusString, sizeof(wchar_t) * std::wcslen(wideRusString));

    return 0;
}

void memoryDumpResearch(void* locationPointer, size_t memorySizeInBytes) {
    char* convertedPointer = reinterpret_cast<char *>(locationPointer);
    
    for (unsigned long shift = 0; shift < memorySizeInBytes; shift++) {
        printf("%02hhX ", *(convertedPointer + shift));
    }
}
