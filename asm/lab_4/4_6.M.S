.extern print_eight_params

.section .text
.globl main
main:
    subq $72, %rsp
    
    movq $1, %rcx
    movq $2, %rdx
    movq $3, %r8
    movq $4, %r9
    movq $5, 32(%rsp)
    movq $6, 40(%rsp)
    movq $7, 48(%rsp)
    movq $8, 56(%rsp)
    
    call print_eight_params
    
    addq $72, %rsp
    movl $0, %eax
    ret