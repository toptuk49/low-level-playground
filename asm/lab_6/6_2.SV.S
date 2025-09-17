.global performBitwiseOperations
.extern explore16BitNumber

performBitwiseOperations:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12

    # Загружаем значение x
    movzwq (%rdi), %rbx  # беззнаковое
    movswq (%rdi), %r12  # знаковое

    # Печать исходного значения
    movw %bx, %di
    call explore16BitNumber

    # Беззнаковые операции
    # a1) беззнаковое умножение на 2
    movw %bx, %ax
    movw $2, %cx
    mulw %cx
    movw %ax, %di
    call explore16BitNumber

    # a2) знаковое умножение на 2
    movw %r12w, %ax
    movw $2, %cx
    imulw %cx
    movw %ax, %di
    call explore16BitNumber

    # a3) беззнаковое деление на 2
    xorw %dx, %dx
    movw %bx, %ax
    movw $2, %cx
    divw %cx
    movw %ax, %di
    call explore16BitNumber

    # a4) знаковое деление на 2
    movw %r12w, %ax
    cwd                 # Расширяем ax -> dx:ax
    movw $2, %cx
    idivw %cx
    movw %ax, %di
    call explore16BitNumber

    # a5) остаток от деления на 16
    xorw %dx, %dx
    movw %bx, %ax
    movw $16, %cx
    divw %cx
    movw %dx, %di
    call explore16BitNumber

    # a6) округление вниз до кратного 16
    movw %bx, %ax
    andw $0xFFF0, %ax
    movw %ax, %di
    call explore16BitNumber

    # б1, б2) сдвиг влево (shl и sal одинаковы)
    movw %bx, %ax
    shlw $1, %ax
    movw %ax, %di
    call explore16BitNumber

    # б3) беззнаковый сдвиг вправо
    movw %bx, %ax
    shrw $1, %ax
    movw %ax, %di
    call explore16BitNumber

    # б4) знаковый сдвиг вправо
    movw %r12w, %ax
    sarw $1, %ax
    movw %ax, %di
    call explore16BitNumber

    # б5) x & 15
    movw %bx, %ax
    andw $15, %ax
    movw %ax, %di
    call explore16BitNumber

    # б6) x & -16
    movw %bx, %ax
    andw $0xFFF0, %ax
    movw %ax, %di
    call explore16BitNumber

    popq %r12
    popq %rbx
    movq %rbp, %rsp
    popq %rbp

    ret