.data
    message:
        .string "Group 2:\Kuznetsov, Lozhkin"

.text
.globl main

main:
    subl $8, %esp
    pushl $message
    call puts
    addl $8, %esp
    movl $0, %eax
    ret
