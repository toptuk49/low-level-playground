section .text
global inc32_asm

inc32_asm:
    ; void inc32_asm(void *p)
    ; rcx - указатель на 32-битное значение
	sub %rsp, 32
    inc dword [%rcx]
	add %rsp, 32
    ret