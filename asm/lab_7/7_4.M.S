section .data
    fmt_out db "Результат: %.15f", 13, 10, 0
    three dq 3.0
    two dq 2.0
    x dq 2.5
    y dq 1.25

section .bss
    result     resq 1

section .text
    global main
    extern printf, exit

main:
    push %rbp
    mov %rbp, %rsp
    sub %rsp, 40

    ; Загрузка значений
    movsd %xmm0, [rel x]
    movsd %xmm1, [rel y]

    ; x/3
    divsd %xmm0, [rel three]

    ; 2/y
    movsd %xmm2, [rel two]
    divsd %xmm2, %xmm1

    ; x*y
    movsd %xmm3, [rel x]
    mulsd %xmm3, %xmm1

    ; Финальное вычисление
    addsd %xmm0, %xmm2
    subsd %xmm0, %xmm3
    movsd [rel result], %xmm0

    ; Вывод результата
    lea %rcx, [rel fmt_out]
    movsd %xmm0, [rel result]
    call printf

    add %rsp, 40
    pop %rbp
    xor %ecx, %ecx
    call exit