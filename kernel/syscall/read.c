#include "syscall.h"
#include "tty.h"
#include "misc.h"

int sys_read(cpu_task_state_t *state, uintptr_t fd, char *buffer, size_t len)
{
    file_t *f = current_task->fds[fd];
    int result = f->ops.read(f, buffer, len);
    if (result == -1)
    {
        await_stdin(state);
    }

    return result;
    // if (f->flags & ~RONLY_FLAG || f->ops->read == NULL)
    // {
    //     return -1;
    // }
    // return f->ops.read(f, buffer, len);
}
