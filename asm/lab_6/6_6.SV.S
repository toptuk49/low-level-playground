.global calculateSum
calculateSum:
    pushq %rbp
    movq %rsp, %rbp

    movl $0, %eax # sum = 0
    testq %rsi, %rsi # N == 0
    jz .end

.loop:
    movl (%rdi), %ecx # %ecx = *nums

    addl %ecx, %eax # sum += *nums
    addq $4, %rdi # nums += sizeof(int)

    decq %rsi # N--
    jnz .loop

.end:
    popq %rbp
    ret