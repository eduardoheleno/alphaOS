#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include "filesystem/fs.h"

#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xC03FF000

#define CURSOR_WIDTH  10
#define CURSOR_HEIGHT 20

#define DEFAULT_WIDTH_SPACING  11
#define DEFAULT_HEIGHT_SPACING 20

#define STDIN_BUFFER_SIZE 4096

#define SET_FLAG_REQUEST   1
#define CLEAR_FLAG_REQUEST 2

#define WONLY_FLAG (0 << 0)
#define RONLY_FLAG (1 << 0)
#define ECHO_FLAG  (1 << 1)

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

void terminal_initialize(void);
void terminal_writestring(const char* data);
void terminal_writeuint(uint32_t value);
void terminal_writehex(uint32_t value);
void terminal_write(const char* data, size_t size);
size_t strlen(const char* str);
void write_tty_buffer(char c);
void terminal_clear(void);
// int stdin_buffer_has_line(void);
file_ops_t tty_ops(void);

#endif
