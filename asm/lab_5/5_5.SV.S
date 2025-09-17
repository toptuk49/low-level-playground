.section .data
x: .int 5 # Входное значение x
z: .int 0 # Результат

.section .text
.global main

main:
    pushq %rbp
    movq %rsp, %rbp

    movl x(%rip), %eax

    leal -2(%eax), %ebx # ebx = -2 + x

    cmpl $-2, %ebx
    jae .greater

    # Если -2 + x < -2, z = 25
    movl $25, %eax
    movl %eax, z(%rip) # Сохраняем 2 в z
    jmp .end

.greater:
    # Если -2 + x >= -2, z = -2 + x
    movl %ebx, %eax # Перемещаем 2x + 1 в eax для возврата
    movl %eax, z(%rip) # Сохраняем 2x + 1 в z

.end:
    movl $0, %eax
    popq %rbp
    ret