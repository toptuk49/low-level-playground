.section .data
    message1:
        .string "%hu %u %llu %f %lf"  
    message2:
        .string "i16: %hu, i32: %u, i64: %llu, f32: %f, f64: %lf\n"  

.section .text
    .globl main

main:
    subq $40, %rsp

    leaq message1(%rip), %rdi
    leaq 30(%rsp), %rsi   #&i16
    leaq 24(%rsp), %rdx   #&i32
    leaq 16(%rsp), %rcx   #&i64
    leaq 8(%rsp), %r8     #&f32
    leaq (%rsp), %r9      #&f64

    call scanf

    leaq message2(%rip), %rdi
    movzwl 30(%rsp), %esi   #i16
    movl 24(%rsp), %edx   #i32
    movq 16(%rsp), %rcx   #i64
    movss 8(%rsp), %xmm0   
    cvtss2sd %xmm0, %xmm0    #float -> double f32
    movsd (%rsp), %xmm1    #f64

    call printf
    
    movl $0, %eax
    addq $40, %rsp
    ret
