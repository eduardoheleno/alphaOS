#ifndef _GETDENTS_H
#define _GETDENTS_H

#include <stdint.h>

struct dir_entry
{
    uint32_t inode_number;
    char name[28];
};

int getdents(int fd, void* buffer);

#endif
