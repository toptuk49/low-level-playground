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
    subq $32, %rsp

    leaq message1(%rip), %rdi          
    leaq -2(%rbp), %rsi #&i16
    leaq -8(%rbp), %rdx #&i32
    leaq -16(%rbp), %rcx #&i64
    leaq -24(%rbp), %r8 #&f32
    leaq -32(%rbp), %r9 #&f64

    call scanf

    leaq message2(%rip), %rdi          
    movzwl -2(%rbp), %esi #i16
    movl -8(%rbp), %edx #i32
    movq -16(%rbp), %rcx #i64
    movss -24(%rbp), %xmm0          
    cvtss2sd %xmm0, %xmm0 #float -> double f32
    movsd -32(%rbp), %xmm1 #f64

    call printf
    
    movl $0, %eax
    addq $32, %rsp
    popq %rbp
    ret
