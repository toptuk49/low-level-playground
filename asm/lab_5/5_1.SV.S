.section .data
x: .int 4294967295 # Максимальное 32-битное беззнаковое число
y: .int 1 # При таком y будет переполнение
z: .int 0 # Результат сложения
w: .int 0 # Результат условия (1 если было переполнение)

.section .text
.global main

main:
    pushq %rbp
    movq %rsp, %rbp

    movl x(%rip), %eax
    addl y(%rip), %eax # Устанавливает флаг OF при переполнении
    movl %eax, z(%rip)
    
    # Определение w на основе флага CF
    seto %al # al = 1 если OF=1, иначе al = 0
    movzbl %al, %eax # Расширяем %al до %eax (заполняем нулями старшие биты)
    movl %eax, w(%rip)

    movl $0, %eax
    popq %rbp
    ret