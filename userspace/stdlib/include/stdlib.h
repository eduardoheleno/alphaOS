#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096

typedef struct page_block page_block_t;
typedef struct mem_block mem_block_t;

struct page_block
{
    size_t og_size;
    size_t free_size;
    void* next_free_addr;
    mem_block_t* free_blocks;
    page_block_t* next_page_block;
    page_block_t* prev_page_block;
};

struct mem_block
{
    size_t size;
    page_block_t* parent_page;
    mem_block_t* next_mem_block;
    _Bool is_allocated;
};

extern page_block_t* mem_pool;

static inline uint32_t pages_required(uint32_t len)
{
    return (len + PAGE_SIZE - 1) / PAGE_SIZE;
}

void* malloc(size_t size);
void free(void* addr);
char* getenv(char* name);

#endif
