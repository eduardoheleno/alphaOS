#include "syscall.h"
#include "scheduler.h"

int sys_execve(const char* path)
{
    return enqueue_task(path);
}
