.text
.globl f1

f1:
    leaq -7(%rdi, %rsi, 8), %rax
    ret