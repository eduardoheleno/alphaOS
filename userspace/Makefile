BUILD_DIR := build

CC := i686-elf-gcc
LD := i686-elf-ld
OBJCOPY := i686-elf-objcopy
AR := i686-elf-ar

CFLAGS := -ffreestanding -nostdlib -fno-pie -m32 -Wall -Wextra

CPPFLAGS := -I ./libc/include \
			-I ./stdlib/include

LIBC_LIB := $(BUILD_DIR)/libc.a
STDLIB_LIB := $(BUILD_DIR)/stdlib.a

SHELL_SRCS := ./programs/shell.c
SHELL_OBJS := $(SHELL_SRCS:%.c=$(BUILD_DIR)/%.o)
SHELL_ELF := $(BUILD_DIR)/shell.elf
SHELL_PROGRAM := $(BUILD_DIR)/shell.bin

TEST_SRCS := ./programs/test.c
TEST_OBJS := $(TEST_SRCS:%.c=$(BUILD_DIR)/%.o)
TEST_ELF := $(BUILD_DIR)/test.elf
TEST_PROGRAM := $(BUILD_DIR)/test.bin

LIBC_SRCS := \
		  ./libc/unistd/exit.c \
		  ./libc/unistd/read.c \
		  ./libc/unistd/write.c \
		  ./libc/stdio/printf.c \
		  ./libc/stdio/fgets.c \
		  ./libc/stdio/getch.c \
		  ./libc/string.c \
		  ./libc/ioctl.c \
		  ./libc/mmap.c \
		  ./libc/munmap.c \
		  ./libc/execve.c

STDLIB_SRCS := \
		  ./stdlib/malloc.c \
		  ./stdlib/free.c

CRT_SRCS := \
		  ./crt/crt0.s

LIBC_OBJS := $(LIBC_SRCS:%.c=$(BUILD_DIR)/%.o)
STDLIB_OBJS := $(STDLIB_SRCS:%.c=$(BUILD_DIR)/%.o)
CRT_OBJS := $(CRT_SRCS:%.s=$(BUILD_DIR)/%.o)
C_OBJS := $(LIBC_OBJS) $(STDLIB_OBJS) $(CRT_OBJS)

.PHONY: all clean

all: $(C_OBJS) $(LIBC_LIB) $(STDLIB_LIB) $(SHELL_PROGRAM) $(TEST_PROGRAM) ../disk.img

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(CRT_OBJS): $(CRT_SRCS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBC_LIB): $(LIBC_OBJS)
	$(AR) rcs $@ $^

$(STDLIB_LIB): $(STDLIB_OBJS)
	$(AR) rcs $@ $^

$(SHELL_PROGRAM): $(SHELL_SRCS) $(LIBC_LIB) $(STDLIB_LIB) $(CRT_OBJS)
	mkdir -p $(BUILD_DIR)/programs
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(SHELL_SRCS) -o $(SHELL_OBJS)
	$(LD) -m elf_i386 -Ttext=0x00001000 -e _start $(CRT_OBJS) $(SHELL_OBJS) $(STDLIB_LIB) $(LIBC_LIB) -o $(SHELL_ELF)
	$(OBJCOPY) -O binary $(SHELL_ELF) $@

$(TEST_PROGRAM): $(TEST_SRCS) $(LIBC_LIB) $(STDLIB_LIB) $(CRT_OBJS)
	mkdir -p $(BUILD_DIR)/programs
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(TEST_SRCS) -o $(TEST_OBJS)
	$(LD) -m elf_i386 -Ttext=0x00001000 -e _start $(CRT_OBJS) $(TEST_OBJS) $(STDLIB_LIB) $(LIBC_LIB) -o $(TEST_ELF)
	$(OBJCOPY) -O binary $(TEST_ELF) $@

../disk.img: $(SHELL_SRCS) $(TEST_SRCS) $(LIBC_LIB) $(STDLIB_LIB) $(CRT_OBJS)
	rm -rf $@
	qemu-img create -f raw $@ 64M

clean:
	rm -rf $(BUILD_DIR)
	rm -rf ../initrd/bin/test
	rm -rf ../initrd/bin/shell
