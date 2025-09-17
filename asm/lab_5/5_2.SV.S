.section .data
x: .int 10 # Тестовое значение x (можно изменить)
z: .int 0 # Результат условия (1 если x ≤ 12)

.section .text
.global main

main:
    pushq %rbp
    movq %rsp, %rbp

    # Сравнение x с 12
    movl x(%rip), %eax
    cmpl $12, %eax

    # Установка z = (x <= 12)
    setle %al # al = 1, если x <= 12 (знаковое сравнение)
    movzbl %al, %eax # Расширяем al до eax, заполняя нулями старшие биты
    movl %eax, z(%rip)

    movl $0, %eax
    popq %rbp
    ret