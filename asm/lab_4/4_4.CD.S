.data
    message1: 
        .string "%hu %u %llu %f %lf"
    message2: 
        .string "i16: %hu, i32: %u, i64: %llu, f32: %f, f64: %lf\n"

.text
.globl main
main:
    subl $60, %esp

    movl $message1, 0(%esp) # fmt
    leal 24(%esp), %eax
    movl %eax, 4(%esp)  # &i16
    leal 28(%esp), %eax
    movl %eax, 8(%esp)  # &i32
    leal 32(%esp), %eax
    movl %eax, 12(%esp)  # &i64
    leal 40(%esp), %eax
    movl %eax, 16(%esp)  # &f32
    leal 44(%esp), %eax
    movl %eax, 20(%esp)  # &f64

    call scanf

    movl $message2, 0(%esp)
    movzwl 24(%esp), %eax # i16
    movl %eax, 4(%esp)
    movl 28(%esp), %eax # i32
    movl %eax, 8(%esp)
    movl 32(%esp), %eax # i64 нижняя часть
    movl %eax, 12(%esp)
    movl 36(%esp), %eax # i64 верхняя часть
    movl %eax, 16(%esp)
    flds 40(%esp) # f32 float -> fpu
    fstpl 20(%esp) # fpu -> double
    movl 44(%esp), %eax # f64 нижняя часть
    movl %eax, 28(%esp)
    movl 48(%esp), %eax # f64 верхняя часть
    movl %eax, 32(%esp)

 // printf(fmt, ushort, uint, ull, float, double)
 call printf
 
 addl $60, %esp # восстановление стека
 movl $0, %eax
 ret
