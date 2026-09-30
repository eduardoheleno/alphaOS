#include "syscall.h"
#include "tty.h"
#include "memory.h"

int sys_getcwd(char* buffer)
{
    kmemcpy(buffer, current_task->cwd, strlen(current_task->cwd));
    buffer[strlen(current_task->cwd)] = '\0';
    return 1;
}
