.section .data
    message1:
        .string "%hu %u %llu %f %lf"
    message2:
        .string "i16: %hu, i32: %u, i64: %llu, f32: %f, f64: %lf\n"

.section .text
    .globl main

main:
    pushq %rbp
    movq %rsp, %rbp
    subq $80, %rsp

    leaq message1(%rip), %rcx
    leaq -2(%rbp), %rdx #&i16
    leaq -8(%rbp), %r8 #&i32
    leaq -16(%rbp), %r9 #&i64
    leaq -24(%rbp), %rax           
    movq %rax, -48(%rbp) #&f32
    leaq -32(%rbp), %rax           
    movq %rax, -40(%rbp) #&f64

    call scanf

    leaq message2(%rip), %rcx
    movzwl -2(%rbp), %edx #i16
    movl -8(%rbp), %r8d #i32
    movq -16(%rbp), %r9 #i64
    movss -24(%rbp), %xmm0          
    movsd -32(%rbp), %xmm1          
    cvtss2sd %xmm0, %xmm0 #float -> double 
    movsd %xmm0, -48(%rbp) #f32
    movsd -32(%rbp), %xmm1
    movsd %xmm1, -40(%rbp) #f64

    call printf
    
    movl $0, %eax    
    addq $80, %rsp
    popq %rbp
    ret