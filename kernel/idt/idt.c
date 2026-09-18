#include "idt.h"

#include "tty.h"
#include "scheduler.h"
#include "syscall.h"
#include "misc.h"

static idt_entry_t idt_entries[256];
static idt_t idt;

extern void *isr_stub_table[];
extern void irq0_stub(void);
extern void irq1_stub(void);
extern void syscall_stub(void);

extern task_t *current_task;

void exception_handler(cpu_exception_state_t *state)
{
    switch (state->exception_code)
    {
        case 0:
            terminal_writestring("Divided by 0!\n");
            break;
        case 14:
            terminal_writestring("Page Fault!\n");
            debug_write("Page Fault!\n");
            if (state->error_code >> 2 == 1)
            {
                terminal_writestring("Terminating userspace process...\n");
                task_exit();
            }
            else
            {
                terminal_writestring("Kernel crash\n");
                __asm__ ("cli; hlt");
            }
            break;
        default:
            terminal_writestring("Exception handler called without error num\n");
            break;
    }
}

void syscall_handler(cpu_task_state_t *state)
{
    switch (state->eax)
    {
        case SYS_EXIT:
            task_exit();
            break;
        case SYS_READ:
            state->eax = sys_read(state, state->ebx, (char*)state->ecx, state->edx);
            break;
        case SYS_WRITE:
            state->eax = sys_write(state->ebx, (char*)state->ecx, state->edx);
            break;
        case SYS_MMAP:
            state->eax = sys_mmap((void*)state->ebx, state->ecx);
            break;
        case SYS_EXECVE:
            state->eax = sys_execve((const char*)state->ebx);
            break;
        case SYS_IOCTL:
            state->eax = sys_ioctl(state->ebx, state->ecx, (void*)state->edx);
            break;
        case SYS_MUNMAP:
            state->eax = sys_munmap((void*)state->ebx, state->ecx);
            break;
        case 2:
            terminal_writestring("test syscall executed\n");
            break;
    }
}

static void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags)
{
    idt_entry_t *descriptor = &idt_entries[vector];

    descriptor->isr_low = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs = 0x08;
    descriptor->attributes = flags;
    descriptor->isr_high = (uint32_t)isr >> 16;
    descriptor->reserved = 0;
}

void idt_init(void)
{
    idt.base = (uintptr_t)&idt_entries[0];
    idt.limit = sizeof(idt_entry_t) * 256 - 1;

    for (uint8_t vector = 0; vector < 32; vector++) 
    {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
    }

    idt_set_descriptor(32, irq0_stub, 0x8E);
    idt_set_descriptor(33, irq1_stub, 0x8E);
    idt_set_descriptor(0x80, syscall_stub, 0xEE);

    __asm__ volatile ("lidt %0" : : "m"(idt));

    terminal_writestring("IDT initialized\n");
}
