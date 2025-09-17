# Как скомпилировать программу?

## Компиляция файлов c помощью GCC

1. Сборка:

```bash
as pow.s -o pow.o
```

2. Линковка: Линкуем с библиотекой libc, так как используются функции printf, scanf, pow и exit. Это делается с помощью ld или gcc:

```bash
ld pow.o -lc -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o pow
# Или с использованием gcc
gcc pow.o -o pow
```

3. Запуск:

```bash
./pow
```

## Компиляция с помощью nasm

```bash
nasm -f win64 <программа>.S
gcc <программа>.obj -o <выходная программа>.exe -lmingwex -lmsvcrt
```
