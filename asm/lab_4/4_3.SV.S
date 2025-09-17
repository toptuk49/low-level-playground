.data
    message:
        .string "Group 2:\Kuznetsov, Lozhkin"

.text
.globl main

main:
    subq $8, %rsp
    leaq message(%rip), %rdi
    call puts
    addq $8, %rsp
    movl $0, %eax 
    ret
