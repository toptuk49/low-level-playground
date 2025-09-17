.section .data
    message1:
        .string "%hu %u %llu %f %lf" 
    message2:
        .string "i16: %hu, i32: %u, i64: %llu, f32: %f, f64: %lf\n"

.section .text
    .globl main

main:
    subq $88, %rsp

    leaq message1(%rip), %rcx 
    leaq 78(%rsp), %rdx  #&i16
    leaq 72(%rsp), %r8   #&i32
    leaq 64(%rsp), %r9   #&i64
    leaq 56(%rsp), %rax   
    movq %rax, 32(%rsp)  #&f32
    leaq 48(%rsp), %rax
    movq %rax, 40(%rsp)  #&f64

    call scanf

    leaq message2(%rip), %rcx 
    movzwl 78(%rsp), %edx  #i16
    movl 72(%rsp), %r8d  #i32
    movq 64(%rsp), %r9   #i64
    movss 56(%rsp), %xmm0
    movsd 48(%rsp), %xmm1
    cvtss2sd %xmm0, %xmm0   #float -> double
    movsd %xmm0, 32(%rsp) #f32
    movsd 48(%rsp), %xmm1 
    movsd %xmm1, 40(%rsp) #f64 

    call printf
    
    movl $0, %eax    
    addq $88, %rsp
    ret