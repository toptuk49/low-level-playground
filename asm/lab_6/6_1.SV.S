.global roundToAMultipleofDInAssembly
.extern explore16BitNumber

roundToAMultipleofDInAssembly:
    pushq %rbp
    movq %rsp, %rbp

    movzwq (%rdi), %rax # rax = x (загружаем 16-битное значение из памяти, на которую указывает rdi, расширяем до 64 бит)

    pushq %rbx # Сохраняем старое значение rbx
    movq %rax, %rbx

    # Печать исходного значения x (rdi = x, преобразуем обратно в 16 бит, чтобы соответствовать explore16BitNumber)
    movw %ax, %di
    callq explore16BitNumber

    # Округление вниз x1 = x & ~0x1F (используем rbx для x)
    movw %bx, %ax
    andw $0xFFE0, %ax
    movw %ax, %di
    callq explore16BitNumber

    # Округление вверх x2 = (x + 31) & ~0x1F (используем rbx для x)
    movw %bx, %ax
    addw $31, %ax
    andw $0xFFE0, %ax
    movw %ax, %di
    callq explore16BitNumber

    popq %rbx
    popq %rbp

    ret