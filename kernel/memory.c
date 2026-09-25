#include "memory.h"

#include "graphics/framebuffer.h"
#include "misc.h"
#include "tty.h"
#include <stddef.h>
#include <stdint.h>

extern char _kernel_start;
extern uintptr_t kernel_page_directory[];

static uintptr_t tmp_map_addr;

static uint16_t slab_classes[] = {
    SLAB_16,
    SLAB_32,
    SLAB_64,
    SLAB_128,
    SLAB_256,
    SLAB_512,
    SLAB_1024,
    SLAB_2048
};

//TODO: implement bitwise operations to truly use a single bit to control pages
static uint8_t kheap_pages[KHEAP_PAGES_NUM] = { 0 };

static slab_cache_t slab_caches[] = {
    { SLAB_16, NULL, NULL },
    { SLAB_32, NULL, NULL },
    { SLAB_64, NULL, NULL },
    { SLAB_128, NULL, NULL },
    { SLAB_256, NULL, NULL },
    { SLAB_512, NULL, NULL },
    { SLAB_1024, NULL, NULL },
    { SLAB_2048, NULL, NULL },
};

static uint8_t *bitmap = NULL;
static uint64_t bitmap_size = 0;

static multiboot_memory_map_t* fetch_highest_block(multiboot_info_t *mbi, multiboot_memory_map_t *mmap)
{
    for (;;) 
    {
        if ((uintptr_t)mmap + mmap->size + sizeof(uint32_t) >= mbi->mmap_addr + mbi->mmap_length) break;
        mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(uint32_t));
    }

    return mmap;
}

static void set_used_bitmaps(uint64_t start_index, uint64_t total_used_pages)
{
    for (uint64_t i = 0; i < total_used_pages; i++) 
    {
        BIT_SET(bitmap, start_index);
        start_index++;
    }
}

static void populate_bitmap(multiboot_info_t *mbi, multiboot_memory_map_t *mmap,
        uintptr_t kernel_end_addr, multiboot_module_t* initrd_module)
{
    bitmap = (uint8_t*)(initrd_module->mod_end + KERNEL_BASE);
    for (uint64_t i = 0; i < bitmap_size; i++) 
    {
        BIT_CLEAR(bitmap, i);
    }

    for (;;) 
    {
        // set used based on GRUB
        if (mmap->type != MULTIBOOT_MEMORY_AVAILABLE) 
        {
            uint64_t start_index = mmap->addr / PAGE_SIZE;
            uint64_t range_size = (mmap->addr + mmap->len) - mmap->addr;
            uint64_t total_used_pages = range_size / PAGE_SIZE;
            if (range_size % PAGE_SIZE > 0) total_used_pages++;
            set_used_bitmaps(start_index, total_used_pages);
        }

        multiboot_memory_map_t *next_mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(uint32_t));
        if ((uintptr_t)next_mmap >= mbi->mmap_addr + mbi->mmap_length) break;

        if (mmap->addr + mmap->len < next_mmap->addr) 
        {
            // set used based on gap
            uint64_t start_index = (mmap->addr + mmap->len) / PAGE_SIZE;
            uint64_t range_size = next_mmap->addr - (mmap->addr + mmap->len);
            uint64_t total_used_pages = range_size / PAGE_SIZE;
            if (range_size % PAGE_SIZE > 0) total_used_pages++;
            set_used_bitmaps(start_index, total_used_pages);
        }
        
        mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(uint32_t));
    }

    // set used based on kernel and bitmap size
    uint64_t start_index = (uint64_t)_kernel_start / PAGE_SIZE;
    uint64_t range_size = kernel_end_addr - (uint64_t)_kernel_start;
    uint64_t total_used_pages = range_size / PAGE_SIZE;
    if (range_size % PAGE_SIZE > 0) total_used_pages++;
    set_used_bitmaps(start_index, total_used_pages);

    // set used based on module
    uint64_t module_start_index = align_down_4k(initrd_module->mod_start) / PAGE_SIZE;
    uint64_t module_final_index = align_up_4k(initrd_module->mod_end) / PAGE_SIZE;
    uint64_t module_total_used_pages = module_final_index - module_start_index;
    set_used_bitmaps(module_start_index, module_total_used_pages);
}

uintptr_t* map_tmp_page(uintptr_t phys_addr)
{
    uintptr_t pde = kernel_page_directory[PDE_INDEX(tmp_map_addr)];
    uint32_t *tmp_page_table = (uint32_t*)((pde & PAGE_MASK) + KERNEL_BASE);
 
    tmp_page_table[PTE_INDEX(tmp_map_addr)] = (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE;

    invlpg(tmp_map_addr);

    return (uintptr_t*)tmp_map_addr;
}

void unmap_tmp_page(void)
{
    uintptr_t pde = kernel_page_directory[PDE_INDEX(tmp_map_addr)];
    uint32_t *tmp_page_table = (uint32_t*)((pde & PAGE_MASK) + KERNEL_BASE);
 
    tmp_page_table[PTE_INDEX(tmp_map_addr)] = 0x0;

    invlpg(tmp_map_addr);
}

uintptr_t pmm_alloc(uint32_t npages)
{
    // TODO: implement check for full physical memory usage
    uint64_t i = 0;
    uint32_t page_accumulator = 0;
    for (; i < bitmap_size; i++) 
    {
        if (BIT_GET(bitmap, i) != BITMAP_FREE)
        {
            page_accumulator = 0;
        }
        else 
        {
            page_accumulator++;
            if (page_accumulator == npages) break;
        }
    }

    for (uint64_t j = i - (npages - 1); j <= i; j++) 
    {
        BIT_SET(bitmap, j);
    }

    return ((i - (npages - 1)) * PAGE_SIZE);
}

void pmm_free(void *addr, uint32_t npages)
{
    // TODO: implement check for non valid addresses
    uint64_t bitmap_idx = (uintptr_t)addr / PAGE_SIZE;
    for (uint64_t i = 0; i < npages; i++) 
    {
        BIT_CLEAR(bitmap, bitmap_idx);
        bitmap_idx++;
    }
}

void init_memory(multiboot_info_t* mbi, unsigned long last_paged_addr,
        uintptr_t *kernel_page_table_idx)
{
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
    multiboot_module_t* mbm = (multiboot_module_t*)mbi->mods_addr;

    multiboot_memory_map_t *highest_block = fetch_highest_block(mbi, mmap);
    uint64_t highest_addr = highest_block->addr + highest_block->len;

    bitmap_size = highest_addr / PAGE_SIZE;
    uintptr_t kernel_end_addr = align_up_4k(last_paged_addr);

    uint64_t bitmap_len = (bitmap_size + 7) / 8;
    uint64_t total_bitmap_pages = (bitmap_len + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t total_module_pages = (align_up_4k(mbm->mod_end) / PAGE_SIZE)
        - (align_down_4k(mbm->mod_start) / PAGE_SIZE);
    for (uint64_t i = 0; i < total_bitmap_pages + total_module_pages; i++) 
    {
        *kernel_page_table_idx = kernel_end_addr | PAGE_PRESENT | PAGE_WRITABLE;

        kernel_end_addr += PAGE_SIZE;
        kernel_page_table_idx++;
    }

    tmp_map_addr = kernel_end_addr + KERNEL_BASE;

    populate_bitmap(mbi, mmap, kernel_end_addr, (multiboot_module_t*)mbi->mods_addr);
    init_video_memory(mbi);

    terminal_writestring("Memory initialized\n");
}

static uint32_t free_virt_area_idx(size_t npages)
{
    uint32_t i = 0;
    uint32_t page_accumulator = 0;
    for (; i < KHEAP_PAGES_NUM; i++) 
    {
        if (kheap_pages[i] != 0) 
        {
            page_accumulator = 0;
        }
        else 
        {
            page_accumulator++;
            if (page_accumulator == npages) break;
        }
    }

    return i - (npages - 1);
}

static uintptr_t mmap(size_t npages, uint8_t flags)
{
    uint32_t free_virt_area_start = free_virt_area_idx(npages);
    uint32_t free_virt_area_tmp = free_virt_area_start;

    for (size_t i = 0; i < npages; i++) 
    {
        uintptr_t page_phys_addr = pmm_alloc(1);
        uintptr_t *new_page = map_tmp_page(page_phys_addr);
        for (uint32_t i = 0; i < 1024; i++)
        {
            new_page[i] = 0x0;
        }
        unmap_tmp_page();

        uintptr_t base_addr = KHEAP_START + (free_virt_area_tmp * PAGE_SIZE);
        if (kernel_page_directory[PDE_INDEX(base_addr)] == 0) 
        {
            uintptr_t page_table_phys_addr = pmm_alloc(1);
            uintptr_t *new_page_table = map_tmp_page(page_table_phys_addr);

            for (uint32_t i = 0; i < 1024; i++) 
            {
                new_page_table[i] = 0x0;
            }
            unmap_tmp_page();

            kernel_page_directory[PDE_INDEX(base_addr)] = (page_table_phys_addr & PAGE_MASK) | flags;
        }

        uintptr_t pde = kernel_page_directory[PDE_INDEX(base_addr)];
        uintptr_t *kernel_page_table = map_tmp_page(pde);

        kernel_page_table[PTE_INDEX(base_addr)] = (page_phys_addr & PAGE_MASK) | flags;

        unmap_tmp_page();
        invlpg(base_addr);

        kheap_pages[free_virt_area_tmp] = 1;
        free_virt_area_tmp++;
    }

    return KHEAP_START + (free_virt_area_start * PAGE_SIZE);
}

static void unmmap(uintptr_t virt_addr, size_t npages)
{
    for (uint32_t i = 0; i < npages; i++) 
    {
        uint32_t page_idx = (virt_addr - KHEAP_START) / PAGE_SIZE;
        uintptr_t pde = kernel_page_directory[PDE_INDEX(virt_addr)];
        uintptr_t *kernel_page_table = map_tmp_page(pde);

        uintptr_t phys_addr = kernel_page_table[PTE_INDEX(virt_addr)] & PAGE_MASK;
        pmm_free((void*) phys_addr, 1);

        kernel_page_table[PTE_INDEX(virt_addr)] = 0x0;
        kheap_pages[page_idx] = 0;

        unmap_tmp_page();
        invlpg(virt_addr);

        virt_addr += PAGE_SIZE;
    }
}

uintptr_t mmap_ring3(uint8_t* program_buffer, size_t size)
{
    uintptr_t page_table_entry_phys_addr1 = pmm_alloc(1);
    uintptr_t* tmp_virt_page_table_entry1 = map_tmp_page(page_table_entry_phys_addr1);
    for (uint16_t i = 0; i < 1024; i++)
    {
        tmp_virt_page_table_entry1[i] = 0x0;
    }

    uint8_t* user_code_virt_addr = (uint8_t*)tmp_virt_page_table_entry1;
    for (uint16_t i = 0; i < size; i++)
    {
        user_code_virt_addr[i] = program_buffer[i];
    }
    unmap_tmp_page();

    uintptr_t page_table_entry_phys_addr2 = pmm_alloc(1);
    uintptr_t* tmp_virt_page_table_entry2 = map_tmp_page(page_table_entry_phys_addr2);
    for (uint16_t i = 0; i < 1024; i++)
    {
        tmp_virt_page_table_entry2[i] = 0x0;
    }
    uint32_t* stack = (uint32_t*)tmp_virt_page_table_entry2;
    stack[PAGE_SIZE / sizeof(*stack) - 1] = 12;
    unmap_tmp_page();

    uintptr_t page_table_phys_addr = pmm_alloc(1);
    uintptr_t* tmp_virt_page_table = map_tmp_page(page_table_phys_addr);
    for (uint16_t i = 0; i < 1024; i++)
    {
        tmp_virt_page_table[i] = 0x0;
    }
    tmp_virt_page_table[1] = (page_table_entry_phys_addr1 & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    tmp_virt_page_table[2] = (page_table_entry_phys_addr2 & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    unmap_tmp_page();

    uintptr_t page_directory_phys_addr = pmm_alloc(1);
    uintptr_t* tmp_virt_page_directory = map_tmp_page(page_directory_phys_addr);
    for (uint16_t i = 0; i < 1024; i++)
    {
        tmp_virt_page_directory[i] = 0x0;
    }
    tmp_virt_page_directory[0] = (page_table_phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    for (uint32_t i = 768; i < 1024; i++)
    {
        tmp_virt_page_directory[i] = kernel_page_directory[i];
    }
    unmap_tmp_page();

    return page_directory_phys_addr;
}

void unmmap_ring3(uintptr_t page_directory_phys_addr)
{
    uint32_t pte_idx = 0;
    uint32_t pt_idx = 1;
    for (; pte_idx < 768; pte_idx++)
    {
        for (; pt_idx < 1024; pt_idx++)
        {
            uintptr_t* tmp_pd = map_tmp_page(page_directory_phys_addr);
            uint32_t pt_addr = tmp_pd[pte_idx];
            unmap_tmp_page();
            if (pt_addr & PAGE_PRESENT)
            {
                uintptr_t* tmp_pt = map_tmp_page(pt_addr);
                uint32_t t_addr = tmp_pt[pt_idx];
                if (t_addr != 0x0)
                {
                    terminal_writestring("table free\n");
                    tmp_pt[pt_idx] = 0x0;
                    pmm_free((void*)(t_addr & PAGE_MASK), 1);
                }
                unmap_tmp_page();
            }

            if (pt_idx + 1 >= 1024)
            {
                uintptr_t* tmp_pd = map_tmp_page(page_directory_phys_addr);
                if (tmp_pd[pte_idx] & PAGE_PRESENT)
                {
                    terminal_writestring("page table free\n");
                    tmp_pd[pte_idx] = 0x0;
                    pmm_free((void*)(pt_addr & PAGE_MASK), 1);
                }
                unmap_tmp_page();
            }
        }

        pt_idx = 0;
    }

    pmm_free((void*)(page_directory_phys_addr & PAGE_MASK), 1);
}

static int8_t size_to_class(size_t size)
{
    for (uint8_t i = 0; i < NUM_CLASSES; i++) 
    {
        if (size <= slab_classes[i]) 
        {
            return i;
        }
    }

    return -1;
}

static void* fp_allocation(size_t size)
{
    size_t npages = ((size + sizeof(slab_t)) / PAGE_SIZE) + 1;
    uintptr_t virt_addr = mmap(npages, PAGE_PRESENT | PAGE_WRITABLE);

    slab_t *big_slab = (slab_t*)virt_addr;
    big_slab->total_count = npages;
    big_slab->cache_owner = NULL;

    return (void*)virt_addr + sizeof(slab_t);
}

void* kmalloc(size_t size)
{
    int8_t slab_class_idx = size_to_class(size);
    if (slab_class_idx < 0) return fp_allocation(size);

    slab_cache_t *slab_cache = &slab_caches[slab_class_idx];
    if (slab_cache->partial != NULL) 
    {
        slab_t *slab = slab_cache->partial;
        void *addr = slab->free_list[slab->free_count-- - 1];
        if (slab->free_count == 0) 
        {
            if (slab->next != NULL) slab->next->prev = NULL;
            slab_cache->partial = slab->next;

            if (slab_cache->full != NULL) slab_cache->full->prev = slab;
            slab->next = slab_cache->full;
            slab->prev = NULL;
            slab_cache->full = slab;
        }

        return addr;
    }

    if (slab_cache->partial == NULL) 
    {
        uintptr_t mapped_virt_addr = mmap(1, PAGE_PRESENT | PAGE_WRITABLE);
        uint16_t total_count = (PAGE_SIZE - sizeof(slab_t)) / slab_cache->obj_size;

        slab_t *new_slab = (slab_t*) mapped_virt_addr;
        new_slab->next = NULL;
        new_slab->prev = NULL;
        new_slab->cache_owner = slab_cache;
        new_slab->total_count = total_count;
        new_slab->free_count = total_count;

        uintptr_t addr_start = mapped_virt_addr + sizeof(slab_t);
        for (uint16_t i = 0; i < new_slab->free_count; i++) 
        {
            new_slab->free_list[i] = (void*) addr_start + (slab_cache->obj_size * i);
        }

        void* addr = new_slab->free_list[--new_slab->free_count];
        if (new_slab->free_count == 0)
        {
            slab_cache->full = new_slab;
        }
        else
        {
            slab_cache->partial = new_slab;
        }

        return addr;
    }

    return NULL;
}

void kfree(void *ptr)
{
    slab_t *slab = (slab_t*)align_down_4k((uintptr_t)ptr);
    if (slab->cache_owner == NULL) 
    {
        unmmap((uintptr_t) slab, slab->total_count);
        return;
    }

    slab->free_list[slab->free_count++] = ptr;
    if (slab->free_count == slab->total_count)
    {
        if (slab->prev != NULL) slab->prev->next = slab->next;
        if (slab->next != NULL) slab->next->prev = slab->prev;

        if (slab->cache_owner->partial == slab)
            slab->cache_owner->partial = slab->next;

        if (slab->cache_owner->full == slab)
            slab->cache_owner->full = slab->next;
        unmmap((uintptr_t)slab, 1);
        return;
    }

    if (slab->free_count == 1)
    {
        if (slab->prev != NULL) slab->prev->next = slab->next;
        if (slab->next != NULL) slab->next->prev = slab->prev;
        if (slab->cache_owner->full == slab)
            slab->cache_owner->full = slab->next;

        if (slab->cache_owner->partial != NULL)
            slab->cache_owner->partial->prev = slab;

        slab->next = slab->cache_owner->partial;
        slab->prev = NULL;
        slab->cache_owner->partial = slab;
    }
}

void* kmemcpy(void* to, void* from, size_t size)
{
    uint8_t* cast_to = (uint8_t*)to;
    uint8_t* cast_from = (uint8_t*)from;
    for (size_t i = 0; i < size; i++)
    {
        cast_to[i] = cast_from[i];
    }

    return to;
}

void* kmemset(void* ptr, int x, size_t n)
{
    uint8_t* cast_ptr = (uint8_t*)ptr;
    for (size_t i = 0; i < n; i++)
    {
        cast_ptr[i] = x;
    }

    return ptr;
}

int kstrcmp(char* ptr1, char* ptr2, size_t num)
{
    for (size_t i = 0; i < num; i++)
    {
        if (ptr1[i] != ptr2[i]) return -1;
    }

    return 0;
}
