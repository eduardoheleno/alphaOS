#include "graphics/framebuffer.h"
#include "memory.h"
#include "misc.h"

static uint32_t framebuffer_width;
static uint32_t framebuffer_height;
static uint32_t framebuffer_pitch;

extern uintptr_t kernel_page_directory[];

void init_video_memory(multiboot_info_t* mbi)
{
    framebuffer_width = mbi->framebuffer_width;
    framebuffer_height = mbi->framebuffer_height;
    framebuffer_pitch = mbi->framebuffer_pitch;

    uint32_t base_pde = PDE_INDEX(KHEAP_END);
    uint32_t base_pte = PTE_INDEX(KHEAP_END);
    uint32_t base_addr = mbi->framebuffer_addr;
    uint32_t pages = (mbi->framebuffer_pitch * mbi->framebuffer_height) / PAGE_SIZE;
    for (uint32_t i = 0; i < pages; i++)
    {
        if (kernel_page_directory[base_pde] == 0x0)
        {
            uintptr_t page_table_phys_addr = pmm_alloc(1);
            uintptr_t* new_page_table = map_tmp_page(page_table_phys_addr);
            for (uint32_t i = 0; i < 1024; i++)
            {
                new_page_table[i] = 0x0;
            }
            unmap_tmp_page();

            kernel_page_directory[base_pde] = (page_table_phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE;
        }

        uintptr_t* kernel_page_table = map_tmp_page(kernel_page_directory[base_pde]);
        kernel_page_table[base_pte] = (base_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE;
        unmap_tmp_page();
        invlpg((base_pde << 22) | (base_pte << 12));

        if (++base_pte >= 1024)
        {
            base_pte = 0;
            base_pde++;
        }
        if (base_pde >= 1024) debug_write("erro");
        base_addr += PAGE_SIZE;
    }
}

void put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x >= framebuffer_width && x < 0) return;
    if (y >= framebuffer_height && y < 0) return;
    
    uint32_t* row = (uint32_t*)(FRAMEBUFFER_ADDR + y * framebuffer_pitch);
    row[x] = color;
}

void clear_framebuffer(void)
{
    for (uint32_t y = 0; y < framebuffer_height; y++)
    {
        for (uint32_t x = 0; x < framebuffer_width; x++)
        {
            put_pixel(x, y, 0x000000);
        }
    }
}
