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
	syscall/execve.c \
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

.PHONY: all initrd clean

all: $(TARGET)
	cp $(TARGET) ../iso/boot/myos
	$(MAKE) initrd
	i686-elf-grub-mkrescue -o ../myos.iso ../iso

$(TARGET): $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%_asm.o: %.s
	mkdir -p $(dir $@)
	$(AS) $< -o $@

initrd:
	cp ../userspace/build/hello.bin ../initrd/bin/hello
	cp ../userspace/build/test.bin ../initrd/bin/test
	tar --format=ustar -cf ../iso/boot/initrd.img -C ../initrd .

clean:
	rm -rf ../myos.iso ../iso/boot/initrd.img
	rm -rf $(BUILD_DIR)
