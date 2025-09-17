#include <stdio.h>
#include <stdint.h>
#include <locale.h>

void explore16BitNumber(void* p);

int main() {
   setlocale(0, "Rus");
   printf("������������ 16-������� �����\n");

   uint16_t value1 = 13;
   uint16_t value2 = 0x8000;

   printf("��� ����� 13:\n");
   explore16BitNumber(&value1);

   printf("\n��� ����� 0x8000:\n");
   explore16BitNumber(&value2);

   return 0;
}

void explore16BitNumber(void* p) {
   short x = *(short*)p;
   unsigned short unsigned_x = *(unsigned short*)p;

   printf("\n����������������� �������������: %X", x);

   printf("\n�������� �������������: ");
   for (int i = 15; i >= 0; i--) {
       printf("%d", (x & (1 << i)) ? 1 : 0);
   }

   printf("\n���������� ����������� �������������: %u", unsigned_x);

   printf("\n���������� �������� �������������: %d", x);

   printf("\n");
}