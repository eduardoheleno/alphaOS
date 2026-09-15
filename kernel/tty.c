#include "tty.h"

#include "memory.h"
#include "scheduler.h"
#include "graphics/font.h"

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

static char stdin_buffer[STDIN_BUFFER_SIZE];
static size_t buffer_head = 0;
static size_t buffer_tail = 0;

static unsigned long flags;

extern task_t *awaiting_stdin;

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	
	for (size_t y = 0; y < VGA_HEIGHT; y++) 
    {
		for (size_t x = 0; x < VGA_WIDTH; x++) 
        {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c)
{
    if (c == '\n') 
    {
        terminal_row++;
        terminal_column = 0;
        return;
    }

    put_char(terminal_column * DEFAULT_WIDTH_SPACING, terminal_row * DEFAULT_HEIGHT_SPACING, 0xAAAAAA, c);
	if (++terminal_column == VGA_WIDTH) 
    {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data)
{
	terminal_write(data, strlen(data));
}

void terminal_writeuint(uint32_t value)
{
    char buffer[11];
    int i = 0;

    if (value == 0) 
    {
        terminal_putchar('0');
        return;
    }

    while (value > 0) 
    {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) 
    {
        terminal_putchar(buffer[--i]);
    }
}

void terminal_writehex(uint32_t value)
{
    char hex_digits[] = "0123456789ABCDEF";

    terminal_putchar('0');
    terminal_putchar('x');

    for (int i = 28; i >= 0; i -= 4)
    {
        uint8_t digit = (value >> i) & 0xF;
        terminal_putchar(hex_digits[digit]);
    }
}

void write_tty_buffer(char c)
{
    if (flags & ECHO_FLAG)
    {
        terminal_write(&c, 1);
    }

    if (awaiting_stdin == NULL) return;

    if (buffer_tail < STDIN_BUFFER_SIZE)
    {
        stdin_buffer[buffer_tail++] = c;
    }
    if (buffer_tail >= STDIN_BUFFER_SIZE && buffer_head == buffer_tail)
    {
        buffer_tail = 0;
        stdin_buffer[buffer_tail++] = c;
    }
    if (c == '\n') wake_stdin_task();
}

int stdin_buffer_has_line(void)
{
    for (size_t i = buffer_head; i < buffer_tail; i++)
    {
        if (stdin_buffer[i] == '\n') return 1;
    }
    return -1;
}

static size_t tty_read(file_t* f, void *buffer, size_t len)
{
    size_t i = 0;
    char *out = buffer;
    for (; i < len; i++)
    {
        if (buffer_head == buffer_tail) break;
        if (buffer_head >= STDIN_BUFFER_SIZE) buffer_head = 0;
        out[i] = stdin_buffer[buffer_head++];
    }
    return i * sizeof(char);
}

static void tty_write(file_t* f, const void *buf, size_t len)
{
    terminal_write(buf, len);
}

static int tty_ioctl(file_t* f, unsigned long request, void *arg)
{
    unsigned long casted_arg = (unsigned long)arg;
    switch (request)
    {
        case SET_FLAG_REQUEST:
            flags |= casted_arg;
            break;
        case CLEAR_FLAG_REQUEST:
            flags &= ~casted_arg;
            break;
    }
    return 1;
}

file_ops_t tty_ops(void)
{
    return (file_ops_t){
        .read = tty_read,
        .write = tty_write,
        .ioctl = NULL,
        .close = NULL
    };
}
