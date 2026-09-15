#include "syscall.h"

int sys_ioctl(uintptr_t fd, unsigned long request, void *arg)
{
    file_t *f = current_task->fds[fd];
    if (f->ops.ioctl == NULL)
    {
        return -1;
    }
    return f->ops.ioctl(f, request, arg);
}
