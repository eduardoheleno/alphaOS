#include "keyboard.h"

#include "pic.h"
#include "misc.h"
#include "tty.h"

static const char *scancodes[] = 
{
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
    NULL, NULL, "\n", NULL,
    "a", "s", "d", "f", "g", "h", "j", "k", "l",
    NULL, NULL, NULL, NULL, NULL,
    "z", "x", "c", "v", "b", "n", "m", ",",
    NULL, NULL, NULL, NULL, NULL, " "
};

// static uint8_t kybrd_ctrl_read_status()
// {
//     return inb(0x64);
// }
//
// static void kybrd_ctrl_send_cmd(uint8_t cmd)
// {
//     while (1) 
//     {
//         if ((kybrd_ctrl_read_status() & KYBRD_CTRL_STATS_MASK_IN_BUF) == 0) break;
//     }
//
//     outb(0x64, cmd);
// }

void keyboard_interrupt_handler(void)
{
    uint8_t scancode = inb(0x60);

    if ((scancode & 0x80) == 0) 
    {
        write_tty_buffer(*scancodes[scancode]);
    }

    pic_send_eoi(1);
}
