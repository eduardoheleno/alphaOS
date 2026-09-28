#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *s);
int strcmp(char* ptr1, char* ptr2, size_t num);
void* memcpy(void* to, void* from, size_t size);
void* memset(void* ptr, int x, size_t n);

#endif
