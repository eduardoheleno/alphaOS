#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>

void exit(void);
int read(unsigned int fd, void *buf, size_t len);
int write(unsigned int fd, const void *buf, size_t len);
int open(const char* path);

#endif

