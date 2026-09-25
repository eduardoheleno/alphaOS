.globl _start
.extern main
.extern exit

_start:
    movl (%esp), %eax
    pushl %eax
    call main
    addl $4, %esp
    call exit
