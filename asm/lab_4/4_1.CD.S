.globl f1

f1:
    movl 4(%esp), %eax
    movl 8(%esp), %ecx 
    leal -7(%eax, %ecx, 8), %eax

    ret
