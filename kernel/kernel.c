#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gdt.h"
#include "idt.h"
#include "memory.h"
#include "pic.h"
#include "timer.h"
#include "filesystem/fs.h"
#include "scheduler.h"
#include "misc.h"
#include "graphics/font.h"
#include "tty.h"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

// #define MULTIBOOT_BOOTLOADER_MAGIC              0x2BADB002

// TODO: improve code organization
// TODO: check memory alignment on kernel allocation
// TODO: test entire flow of malloc
void kernel_main(
        uintptr_t *kernel_page_table_idx,
        unsigned long last_paged_addr,
        unsigned long magic,
        unsigned long mbi_addr
     )
{
    multiboot_info_t* mbi = (multiboot_info_t*)mbi_addr;

    load_psf2_font();
    init_memory(mbi, last_paged_addr, kernel_page_table_idx);
    gdt_init();
    idt_init();
    pic_init(0x20, 0x28);
    pit_init();
    init_fs(mbi);
    init_scheduler();
    enable_interrupts();

    unmap_identity();
    reload_cr3();

    file_t* f = open_file("/bin/test.txt");
    void* test = kmalloc(2000);
    debug_int(f->ops.read(f, test, f->inode.size));
    terminal_writestring(test);

    for (;;) 
    {
        halt();
    }
}
