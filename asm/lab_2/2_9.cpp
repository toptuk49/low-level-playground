#include <stdio.h>
#include <stdint.h>
#include <locale.h>

void explore16BitNumber(void* p) {
    short x = *(short*)p;
    unsigned short unsigned_x = *(unsigned short*)p;

    printf("\nШестнадцатеричное представление: %X", x);

    printf("\nДвоичное представление: ");
    for (int i = 15; i >= 0; i--) {
        printf("%d", (x & (1 << i)) ? 1 : 0);
    }

    printf("\nДесятичное беззнаковое представление: %u", unsigned_x);

    printf("\nДесятичное знаковое представление: %d", x);

    printf("\n");
}

void ab16(void* p) {
    int16_t sx = *((int16_t*)p);
    uint16_t ux = *((uint16_t*)p); // Переводим в беззнаковое представление

    explore16BitNumber(p);

    // а1) Беззнаковое умножение на 2
    uint16_t ux_mult2 = ux * 2;
    printf("  a1) ux * 2 = %u\n", ux_mult2);

    // а2) Знаковое умножение на 2
    int16_t sx_mult2 = sx * 2;
    printf("  a2) sx * 2 = %d\n", sx_mult2);

    // а3) Беззнаковое деление на 2
    uint16_t ux_div2 = ux / 2;
    printf("  a3) ux / 2 = %u\n", ux_div2);

    // а4) Знаковое деление на 2
    int16_t sx_div2 = sx / 2;
    printf("  a4) sx / 2 = %d\n", sx_div2);

    // а5) Расчёт остатка от беззнакового деления на 16
    uint16_t ux_mod16 = ux % 16;
    printf("  a5) ux %% 16 = %u\n", ux_mod16);

    // а6) Округление вниз до числа, кратного 16 (беззнаковое)
    uint16_t ux_round16 = ux - (ux % 16);
    printf("  a6) ux округлено вниз до числа, кратного 16 = %u\n", ux_round16);

    // б1) Беззнаковый << 1
    uint16_t ux_lshift1 = ux << 1;
    printf("  б1) ux << 1 = %u\n", ux_lshift1);

    // б2) Знаковый << 1
    int16_t sx_lshift1 = sx << 1;
    printf("  б2) sx << 1 = %d\n", sx_lshift1);

    // б3) Беззнаковый >> 1
    uint16_t ux_rshift1 = ux >> 1;
    printf("  б2) ux >> 1 = %u\n", ux_rshift1);

    // б4) Знаковый >> 1
    int16_t sx_rshift1 = sx >> 1;
    printf("  б4) sx >> 1 = %d\n", sx_rshift1);

    // б5) X & 15
    int16_t sx_and15 = sx & 15;
    printf("  б5) sx & 15 = %d\n", sx_and15);

    // б6) X & -16
    int16_t sx_and_neg16 = sx & -16;
    printf("  б6) sx & -16 = %d\n", sx_and_neg16);
}

int main() {
    setlocale(0, "Rus");

    int16_t m = 37;
    int16_t n = -33;

    printf("Для значения m = 37:\n");
    ab16(&m);

    printf("\nДля значения n = -33:\n");
    ab16(&n);

    return 0;
}
