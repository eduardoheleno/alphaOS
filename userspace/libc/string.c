#include "string.h"

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
        len++;

    return len;
}

int strcmp(char* ptr1, char* ptr2, size_t num)
{
    for (size_t i = 0; i < num; i++)
    {
        if (ptr1[i] != ptr2[i]) return -1;
    }

    return 0;
}

void* memcpy(void* to, void* from, size_t size)
{
    uint8_t* cast_to = (uint8_t*)to;
    uint8_t* cast_from = (uint8_t*)from;
    for (size_t i = 0; i < size; i++)
    {
        cast_to[i] = cast_from[i];
    }

    return to;
}

void* memset(void* ptr, int x, size_t n)
{
    uint8_t* cast_ptr = (uint8_t*)ptr;
    for (size_t i = 0; i < n; i++)
    {
        cast_ptr[i] = x;
    }

    return ptr;
}
