#include <stdio.h>
#include <stdint.h>
#include <locale.h>

void c16to32(void* p);
void explore16BitNumber(void* p);
void explore32BitNumber(void* p);

int main() {
   setlocale(0, "Rus");

   int16_t m = 37;
   int16_t n = -33;

   printf("������� 3");

   printf("\n�������� ����� m = 37 � ��� �������������:\n");
   c16to32(&m);

   printf("\n�������� ����� n = - 33 � ��� �������������:\n");
   c16to32(&n);

   return 0;
}

void c16to32(void* p) {
   int16_t val16 = *((int16_t*)p);
   explore16BitNumber(p);

   // �������� ���������� (�� short � int)
   int32_t signed_extended = (int32_t)val16; // ������� �������� ����������
   printf("\n����������� ��������:\n");
   explore32BitNumber(&signed_extended);

   // Unsigned Extension (�� unsigned short � unsigned int)
   uint16_t uval16 = *((uint16_t*)p); // ��������� � �����������
   uint32_t unsigned_extended = (uint32_t)uval16; // ������� ����������� ����������
   printf("\n����������� �����������:\n");
   explore32BitNumber(&unsigned_extended);
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
