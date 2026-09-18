#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>

static const int stdin = 0;
static const int stdout = 1;

void printf(const char *s, ...);
void fgets(char *buffer, size_t len, int fd);
int getch(void);

#endif
