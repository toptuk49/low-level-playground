section .data
    fmt_out db "Sum: %.15f", 13, 10, 0
    array dq 1.1, 2.2, 3.3, 4.4, 5.5
    arr_len equ ($ - array) / 8

section .text
    global main
    extern printf, exit

; Вычисление суммы массива double
array_sum:
    xorpd %xmm0, %xmm0 ; Обнуляем аккумулятор
    test %rdx, %rdx ; Проверка на пустой массив
    jz .end
    
.loop:
    addsd %xmm0, [%rcx] ; Добавляем элемент к сумме
    add %rcx, 8 ; Следующий элемент
    dec %rdx ; Уменьшаем счетчик
    jnz .loop ; Продолжаем цикл
    
.end:
    ret

main:
	push %rbp
    mov %rbp, %rsp
    sub %rsp, 48
    
    ; Вычисление суммы (используем относительную адресацию)
    lea %rcx, [rel array] ; Указатель на массив
    mov %rdx, arr_len ; Количество элементов
    call array_sum
    
    ; Вывод результата
    lea %rcx, [rel fmt_out] ; Форматная строка
    movsd %xmm0, [rel array]
    call printf
    
    ; Завершение
	add %rsp, 48
    pop %rbp
    xor %ecx, %ecx
    call exit