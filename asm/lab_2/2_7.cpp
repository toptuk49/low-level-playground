#include <stdio.h>
#include <stdint.h>
#include <locale.h>

void explore64BitNumber(void* p);

int main() {
   setlocale(0, "Rus");
   printf("������������ 32-������� �����\n");

   uint64_t value1 = 13;
   uint64_t value2 = 0x8000000000000000;

   printf("��� ����� 13:\n");
   explore64BitNumber(&value1);

   printf("\n��� ����� 0x8000000000000000:\n");
   explore64BitNumber(&value2);

   return 0;
}

void explore64BitNumber(void* p) {
   /* �� - �� �������� � �������� ������ � GCC � MSVC, long ��������
   4 �����, ������ ������, ������� �������� �� ��������� �������
   � Windows.
   � GCC ������� ��������� long long �� lon*/
   long long x = *(long long*)p;
   unsigned long long unsigned_x = *(unsigned long long*)p;

   printf("\n����������������� �������������: %16X", x);

   printf("\n�������� �������������: ");
   for (int i = 63; i >= 0; i--) {
       printf("%d", (x & (1 << i)) ? 1 : 0);
   }

   printf("\n���������� ����������� �������������: %llu", unsigned_x);

   printf("\n���������� �������� �������������: %+lld", x);

   printf("\n����������������� ���������������� �������������: %+3.3A", x);

   printf("\n���������� ���������������� �������������: %+.2e", x);

   printf("\n������������� � ���������� �������: %+.2f", x);

   printf("\n");
}