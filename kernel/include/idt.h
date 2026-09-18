#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

#include <stdint.h>

#define SYS_EXIT    1
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_MMAP    9
#define SYS_EXECVE 11
#define SYS_BRK    45
#define SYS_IOCTL  54
#define SYS_MUNMAP 91

struct idt_entry
{
	uint16_t    isr_low;
	uint16_t    kernel_cs;
	uint8_t     reserved;
	uint8_t     attributes;
	uint16_t    isr_high;
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

struct idt
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));
typedef struct idt idt_t;

void idt_init(void);

#endif
