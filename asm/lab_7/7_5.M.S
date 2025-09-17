section .data
    prompt_x db "Введите x (основание): ", 0
    prompt_y db "Введите y (степень): ", 0
    fmt_in db "%lf", 0
    fmt_pow db "pow(x, y) = %.15f", 13, 10, 0
    
section .bss
    x resq 1
    y resq 1
    z resq 1

section .text
    global main
    extern printf, scanf, pow, exit

main:
    push %rbp
    mov %rbp, %rsp
    sub %rsp, 48

    ; Ввод x
    lea %rcx, [rel prompt_x]
    call printf

    lea %rcx, [rel fmt_in]
    lea %rdx, [rel x]
    call scanf

    ; Ввод y
    lea %rcx, [rel prompt_y]
    call printf

    lea %rcx, [rel fmt_in]
    lea %rdx, [rel y]
    call scanf

    ; Вычисление pow(x,y)
    movsd %xmm0, [rel x]
    movsd %xmm1, [rel y]
    call pow
    movsd [rel z], %xmm0

    ; Вывод pow(x,y)
    lea %rcx, [rel fmt_pow]
    movsd %xmm0, [rel z]
    call printf

    add %rsp, 48
    pop %rbp
    xor %ecx, %ecx
    call exit