#include <stdio.h>
#include <stdint.h>
#include <locale.h>

void explore32BitNumber(void* p);

int main() {
   setlocale(0, "Rus");
   printf("������������ 32-������� �����\n");

   uint32_t value1 = 13;
   uint32_t value2 = 0x80000000;

   printf("��� ����� 13:\n");
   explore32BitNumber(&value1);

   printf("\n��� ����� 0x80000000:\n");
   explore32BitNumber(&value2);

   return 0;
}

void explore32BitNumber(void* p) {
   int x = *(int*)p;
   unsigned unsigned_x = *(unsigned*)p;

   printf("\n����������������� �������������: %08X", x);

   printf("\n�������� �������������: ");
   for (int i = 31; i >= 0; i--) {
       printf("%d", (x & (1 << i)) ? 1 : 0);
   }

   printf("\n���������� ����������� �������������: %u", unsigned_x);

   printf("\n���������� �������� �������������: %+d", x);

   printf("\n����������������� ���������������� �������������: %+3.3A", x);

   printf("\n���������� ���������������� �������������: %.2e", x);

   printf("\n������������� � ���������� �������: %+.2f", x);

   printf("\n");
}