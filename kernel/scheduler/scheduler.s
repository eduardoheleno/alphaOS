.globl restore_task_context
restore_task_context:
    movl 4(%esp), %eax
    movl 16(%eax), %edx

    movw 0(%eax), %cx
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs

    testb $3, 40(%eax)
    jz .Lkernel_frame

    movl %edx, %esp
    pushl 0(%eax)
    pushl %edx
    jmp .Lcommon_frame

.Lkernel_frame:
    movl %edx, %esp

.Lcommon_frame:
    pushl 44(%eax)
    pushl 40(%eax)
    pushl 36(%eax)

    movl 4(%eax), %edi
    movl 8(%eax), %esi
    movl 12(%eax), %ebp
    movl 20(%eax), %ebx
    movl 24(%eax), %edx
    movl 28(%eax), %ecx
    movl 32(%eax), %eax
    iret

.globl enter_userspace
enter_userspace:
    movl 4(%esp), %eax     # user entry
    movl 8(%esp), %edx     # user stack top

    movw $0x1B, %cx        # USER_DS
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs

    pushl $0x1B            # user SS
    pushl %edx             # user ESP
    pushl $0x002           # EFLAGS
    pushl $0x23            # user CS
    pushl %eax             # user EIP

    iret
