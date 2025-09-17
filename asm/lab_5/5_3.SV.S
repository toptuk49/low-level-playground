.section .data
x: .int -5 # Тестовое значение знакового x (можно изменить)
z: .int 0 # Результат условия (1 если x <= 12)

.section .text
.global main

main:
    pushq %rbp
    movq %rsp, %rbp

    movl x(%rip), %eax

    cmpl $12, %eax

    setle %al        # al = 1 если x <= 12 (знаковое)
    movzbl %al, %eax # Расширяем al до eax, заполняя нулями старшие биты
    movl %eax, z(%rip) # Сохраняем результат

    movl $0, %eax
    popq %rbp
    ret