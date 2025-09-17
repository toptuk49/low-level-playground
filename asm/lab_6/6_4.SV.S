.global divComputation
divComputation:
    pushq %rbp
    movq %rsp, %rbp

    # Сохраняем callee-saved регистры
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14

    # edi = x
    # esi = y
    # rdx =  z_unsigned
    # rcx =  w_unsigned
    # r8 =  z_signed
    # r9 = на w_signed

    # Сохраняем указатели
    movq %rdx, %rbx # z_unsigned
    movq %rcx, %r12 # w_unsigned
    movq %r8, %r13 # z_signed
    movq %r9, %r14 # w_signed

    # (y - 3)
    movl %esi, %eax
    subl $3, %eax # eax = y - 3
    movl %eax, %ecx

    # Беззнаковое
    movl %edi, %eax # eax = x
    xorl %edx, %edx # edx to zero
    divl %ecx # eax = x / (y-3), edx = x % (y-3)
    movl %eax, (%rbx) # z_unsigned
    movl %edx, (%r12) # w_unsigned

    # Знаковое
    movl %edi, %eax # eax = x
    cltd # eax -> edx:eax
    idivl %ecx # eax = x / (y-3), edx = x % (y-3)
    movl %eax, (%r13) # z_signed
    movl %edx, (%r14) #  w_signed

    popq %r14
    popq %r13
    popq %r12
    popq %rbx

    movq %rbp, %rsp
    popq %rbp
    ret