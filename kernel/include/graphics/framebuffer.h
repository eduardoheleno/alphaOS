#ifndef _GRAPHICS_FRAMEBUFFER_H
#define _GRAPHICS_FRAMEBUFFER_H

#include "multiboot.h"

#define FRAMEBUFFER_ADDR 0xE0000000 

void init_video_memory(multiboot_info_t* mbi);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
void clear_framebuffer(void);

#endif
