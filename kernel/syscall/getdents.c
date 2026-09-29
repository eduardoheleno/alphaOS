#include "syscall.h"

int sys_getdents(uintptr_t fd, void* buffer)
{
    file_t* f = current_task->fds[fd];
    if (f->inode.type != DIR_TYPE)
        return -1;
    current_task->fds[fd]->ops.read(f, buffer, f->inode.size);
    return f->inode.size / sizeof(struct dir_entry);
}
