#include "syscall.h"
#include "scheduler.h"

int sys_execve(const char* path, const char* arg)
{
    return enqueue_task(path, arg);
}
