.section .bss
arg:
    .space 4

.globl envp
.globl _start
.extern main
.extern exit

.section .text
_start:
    movl (%esp), %eax
    movl %eax, arg
    call main
    call exit
