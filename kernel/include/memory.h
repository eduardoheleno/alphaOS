#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include "multiboot.h"

#define KERNEL_BASE   0xC0000000
#define KHEAP_START   0xC0400000
#define KHEAP_END     0xE0000000

#define USER_CODE     0x00001000
#define USER_STACK    0x00002000

#define PAGE_MASK     0xFFFFF000
#define PAGE_PRESENT  0x001
#define PAGE_WRITABLE 0x002
#define PAGE_USER     0x004
#define PAGE_SIZE     4096

#define BITMAP_USED   1
#define BITMAP_FREE   0

#define PDE_INDEX(addr) (((addr) >> 22) & 0x3FF)
#define PTE_INDEX(addr) (((addr) >> 12) & 0x3FF)

#define BIT_SET(arr, i)   ((arr)[(i) / 8] |=  (1u << ((i) % 8)))
#define BIT_CLEAR(arr, i) ((arr)[(i) / 8] &= ~(1u << ((i) % 8)))
#define BIT_GET(arr, i)   (((arr)[(i) / 8] >> ((i) % 8)) & 1u)

#define KHEAP_PAGES_NUM (KHEAP_END - KHEAP_START) / PAGE_SIZE

#define SLAB_16       16
#define SLAB_32       32
#define SLAB_64       64
#define SLAB_128      128
#define SLAB_256      256
#define SLAB_512      512
#define SLAB_1024     1024
#define SLAB_2048     2048

#define NUM_CLASSES   8

struct slab
{
    struct slab *next;
    struct slab *prev;

    struct slab_cache *cache_owner;

    uint16_t total_count;
    uint16_t free_count;
    void *free_list[];
};
typedef struct slab slab_t;

struct slab_cache
{
    size_t obj_size;

    slab_t *partial;
    slab_t *full;
    // In the current state of the allocator its not possible to have an empty
    // slab, ALL empty slabs are freed.
    // Maybe the allocator should keep some minimum amount of empty slabs
    // to prevent virtual mapping.
    // slab_t *empty;
};
typedef struct slab_cache slab_cache_t;

void init_memory(multiboot_info_t* mbi, unsigned long last_paged_addr, uintptr_t *kernel_page_table_idx);

uintptr_t pmm_alloc(uint32_t npages);
void pmm_free(void *addr, uint32_t npages);
uintptr_t* map_tmp_page(uintptr_t phys_addr);
void unmap_tmp_page(void);

void* kmalloc(size_t size);
void kfree(void *ptr);
void* kmemcpy(void* to, void* from, size_t size);
void* kmemset(void* ptr, int x, size_t n);
int kstrcmp(char* ptr1, char* ptr2, size_t num);

uintptr_t mmap_ring3(uint8_t* program_buffer, size_t size);
void unmmap_ring3(uintptr_t page_directory_phys_addr);

#endif
