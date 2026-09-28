.section .bss
envp:
    .space 4

.globl envp
.globl _start
.extern main
.extern exit

.section .text
_start:
    movl (%esp), %eax
    movl %eax, envp
    call main
    call exit
