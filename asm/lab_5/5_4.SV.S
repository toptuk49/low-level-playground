.section .data
x:      .double 3.14 # Тестовое значение double (можно изменить)
limit:  .double 12.0 # Граничное значение для сравнения
z:      .int 0 # Результат условия (1 если x <= 12.0)

.section .text
.global main

main:
    pushq %rbp
    movq %rsp, %rbp

    vmovsd x(%rip), %xmm0
    vmovsd limit(%rip), %xmm1

    vcomisd %xmm1, %xmm0 # x <= 12.0?

    setbe %al # al = 1 если x <= 12.0 (беззнаковое для FP)
    movzbl %al, %eax # Расширяем al до eax, заполняя нулями старшие биты
    movl %eax, z(%rip)

    movl $0, %eax
    popq %rbp
    ret