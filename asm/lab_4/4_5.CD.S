.data
    message1: 
        .string "%hu %u %llu %f %lf"
    message2: 
        .string "i16: %hu, i32: %u, i64: %llu, f32: %f, f64: %lf\n"

.text
.globl main
main:              
    movl %esp, %ebp
    pushl %ebp
    subl $56, %esp         

    movl $message1, -60(%ebp) # fmt
    leal -36(%ebp), %eax
    movl %eax, -56(%ebp) # &i16
    leal -32(%ebp), %eax
    movl %eax, -52(%ebp) # &i32
    leal -28(%ebp), %eax
    movl %eax, -48(%ebp) # &i64
    leal -20(%ebp), %eax
    movl %eax, -44(%ebp) # &f32
    leal -16(%ebp), %eax
    movl %eax, -40(%ebp) # &f64

    call scanf

    movl $message2, -60(%ebp)
    movzwl -36(%ebp), %eax # i16
    movl %eax, -56(%ebp)
    movl -32(%ebp), %eax # i32
    movl %eax, -52(%ebp)
    movl -28(%ebp), %eax # i64 нижняя часть
    movl %eax, -48(%ebp)
    movl -24(%ebp), %eax # i64 верхняя часть
    movl %eax, -44(%ebp)
    flds -20(%ebp) # f32 float -> fpu
    fstpl -40(%ebp) # fpu -> double
    movl -16(%ebp), %eax # f64 нижняя часть
    movl %eax, -32(%ebp)
    movl -12(%ebp), %eax # f64 верхняя часть
    movl %eax, -28(%ebp)

    call printf
 
    addl $56, %esp      
    popl %ebp              
    movl $0, %eax
    ret
