.global calculateZ

calculateZ:
    pushq %rbp
    movq %rsp, %rbp

    movl %edi, %eax # eax = x
    movl %esi, %ecx # ecx = y

    # Вычисление x + x * y - 7
    imull %ecx, %eax
    addl %edi, %eax
    subl $7, %eax

    popq %rbp
    ret