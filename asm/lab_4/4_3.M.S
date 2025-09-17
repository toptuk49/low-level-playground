.data
    message:
        .string "Group 2:\Kuznetsov, Lozhkin"

.text
.globl main

main:
    subq $40, %rsp
    leaq message(%rip), %rcx
    call puts
    addq $40, %rsp
    movl $0, %eax
    ret
