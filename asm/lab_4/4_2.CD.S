.text
.globl f2

f2:
    vmovsd 4(%esp), %xmm0
    vmovsd 12(%esp), %xmm1

    vsubsd %xmm1, %xmm0, %xmm0

    ret
