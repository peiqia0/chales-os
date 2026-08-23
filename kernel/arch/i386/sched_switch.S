.global sched_switch
.type sched_switch, @function
sched_switch:
    movl 4(%esp), %eax    # prev
    movl 8(%esp), %edx    # next

    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp

    movl %esp, (%eax)
    movl (%edx), %esp

    popl %ebp
    popl %edi
    popl %esi
    popl %ebx

    ret
