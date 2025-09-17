.global calculateSecondZ
.text
calculateSecondZ:
    lea %rax, [%rdi*4 + %rdi + 5]
    ret