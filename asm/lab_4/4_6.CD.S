.extern print_eight_params

.section .text
.globl main
main:
    subl $36, %esp
    
    movl $8, 28(%esp)
    movl $7, 24(%esp)
    movl $6, 20(%esp)
    movl $5, 16(%esp)
    movl $4, 12(%esp)
    movl $3, 8(%esp)
    movl $2, 4(%esp)
    movl $1, (%esp)

    call print_eight_params

    addl $36, %esp
    
    movl $0, %eax
    ret
