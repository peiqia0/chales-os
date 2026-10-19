.section .text
.global _start
.extern main
.extern exit

_start:
    pop %eax # argc
    mov %esp, %ebx # argv
    lea 4(%esp,%eax,4), %edx   # edx = &envp[0] (one slot past argv's NULL terminator)

    push %edx # envp
    push %ebx # argv
    push %eax # argc
    call main
    add $12, %esp

    push %eax # main's return value
    call exit # should not return

    # exit() does not return
1:
    hlt
    jmp 1b
