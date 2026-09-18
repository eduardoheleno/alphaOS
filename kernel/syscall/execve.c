#include "syscall.h"
#include "scheduler.h"

int sys_execve(const char *path)
{
    enqueue_task(path);
    return 1;
}
