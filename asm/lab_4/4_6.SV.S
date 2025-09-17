.extern print_eight_params

.section .text
.globl main
main:
    subq $24, %rsp
    
    movq $1, %rdi
    movq $2, %rsi
    movq $3, %rdx
    movq $4, %rcx
    movq $5, %r8
    movq $6, %r9
    movq $8, 8(%rsp)
    movq $7, 0(%rsp)
    
    call print_eight_params

    addq $24, %rsp
    movl $0, %eax
    ret
