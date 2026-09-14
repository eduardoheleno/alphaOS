BUILD_DIR := build
TARGET := $(BUILD_DIR)/myos

CC := i686-elf-gcc
AS := i686-elf-as

CFLAGS := -std=gnu99 \
		  -ffreestanding \
		  -O2 \
		  -Wall \
		  -Wextra

CPPFLAGS := -I./include \
		    -I../drivers/include

LDFLAGS := -T linker.ld \
		   -ffreestanding \
		   -O2 \
		   -nostdlib

C_SRCS := \
	kernel.c \
	tty.c \
	memory.c \
	pic.c \
	timer.c \
	filesystem/fs.c \
	filesystem/disk.c \
	filesystem/device.c \
	gdt/gdt.c \
	idt/idt.c \
	scheduler/scheduler.c \
	syscall/mmap.c \
	syscall/munmap.c \
	syscall/read.c \
	syscall/write.c \
	syscall/ioctl.c \
	graphics/framebuffer.c \
	graphics/font.c

ASM_SRCS := \
	boot.s \
	gdt/gdt.s \
	idt/idt.s \
	scheduler/scheduler.s

C_OBJS := $(C_SRCS:%.c=$(BUILD_DIR)/%.o)
ASM_OBJS := $(ASM_SRCS:%.s=$(BUILD_DIR)/%_asm.o)

OBJS := $(ASM_OBJS) $(C_OBJS) ../drivers/build/keyboard.o ../initrd/fonts/font.o

.PHONY: all run clean

$(TARGET): $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	cp $@ ../iso/boot/myos
	cp ../userspace/build/hello.bin ../initrd/bin/hello
	i686-elf-grub-mkrescue -o ../myos.iso ../iso

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%_asm.o: %.s
	mkdir -p $(dir $@)
	$(AS) $< -o $@

initrd:
	tar --format=ustar -cf ./build/initrd.img -C ../initrd .
	cp ./build/initrd.img ../iso/boot/initrd.img

clean:
	rm ../myos.iso
	rm -rf $(BUILD_DIR)
