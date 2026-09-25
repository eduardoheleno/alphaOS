.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set VIDEO,    1<<2

.set FLAGS,    ALIGN | MEMINFO | VIDEO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

.set MODE_TYPE, 0
.set FB_WIDTH,  1024
.set FB_HEIGHT, 768
.set FB_DEPTH,  32

.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.long 0
.long 0
.long 0
.long 0
.long 0

.long MODE_TYPE
.long FB_WIDTH
.long FB_HEIGHT
.long FB_DEPTH

.section .bootstrap_stack, "aw", @nobits
stack_bottom:
    .skip 16384 # 16 KiB
stack_top:

.section .bss, "aw", @nobits
.global kernel_page_directory
.align 4096
kernel_page_directory:
	.skip 4096
boot_page_table:
	.skip 4096

.section .multiboot.text, "a"

readonly_page:
    orl $0x001, %edx
    jmp 2f

.global _start
.type _start, @function
_start:
	movl $(boot_page_table - 0xC0000000), %edi
	movl $0, %esi
	movl $1023, %ecx

1:
	# Only map the kernel.
	cmpl $_kernel_start, %esi
	jl 3f # esi < _kernel_start
	cmpl $_kernel_end, %esi
	jge 4f # esi >= _kernel_end

	movl %esi, %edx

	orl $0x003, %edx
2:	movl %edx, (%edi)

3:
	# Size of page is 4096 bytes.
	addl $4096, %esi
	# Size of entries in boot_page_table is 4 bytes.
	addl $4, %edi
	# Loop to the next entry if we haven't finished.
	loop 1b

4:
	# Map VGA video memory to 0xC03FF000 as "present, writable".
	movl $(0x000B8000 | 0x003), boot_page_table - 0xC0000000 + 1023 * 4

	# The page table is used at both page directory entry 0 (virtually from 0x0
	# to 0x3FFFFF) (thus identity mapping the kernel) and page directory entry
	# 768 (virtually from 0xC0000000 to 0xC03FFFFF) (thus mapping it in the
	# higher half). The kernel is identity mapped because enabling paging does
	# not change the next instruction, which continues to be physical. The CPU
	# would instead page fault if there was no identity mapping.

	# Map the page table to both virtual addresses 0x00000000 and 0xC0000000.
	movl $(boot_page_table - 0xC0000000 + 0x003), kernel_page_directory - 0xC0000000 + 0
	movl $(boot_page_table - 0xC0000000 + 0x003), kernel_page_directory - 0xC0000000 + 768 * 4

	# Set cr3 to the address of the kernel_page_directory.
	movl $(kernel_page_directory - 0xC0000000), %ecx
	movl %ecx, %cr3

	# Enable paging and the write-protect bit.
	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

    jmp 5f

.section .text

5:
	# At this point, paging is fully set up and enabled.

	# Reload cr3 to force a TLB flush so the changes to take effect.
	movl %cr3, %ecx
	movl %ecx, %cr3

	# Set up the stack.
	mov $stack_top, %esp

	# Enter the high-level kernel.
    push %ebx
    push %eax
    push %esi
    push %edi
	call kernel_main

	# Infinite loop if the system has nothing more to do.
	cli
1:	hlt
	jmp 1b
