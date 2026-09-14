#include "syscall.h"
#include "tty.h"

int sys_write(uintptr_t fd, const void *buffer, size_t len)
{
    file_t *f = current_task->fds[fd];
    f->ops.write(buffer, len);
    // if (f->flags & ~WONLY_FLAG || f->ops->write == NULL)
    // {
    //     return -1;
    // }
    // TODO: write function must return the amount of bytes
    // f->ops->write(buffer, len);
    return sizeof(char) * strlen(buffer);
}
