.section .text
.global _start
.extern main
.global exit

_start:
    pop %eax # argc
    mov %esp, %ebx # argv
    lea 4(%esp,%eax,4), %edx   # edx = &argv[argc]
    add $4, %edx # edx = envp[0]

    push %edx # envp
    push %ebx # argv
    push %eax # argc
    call main
    add $12, %esp

    push %eax # main's return value
    call exit # should not return

exit:
    mov 4(%esp), %ebx
    mov $0, %eax
    int $0x80

1:
    hlt
    jmp 1b
