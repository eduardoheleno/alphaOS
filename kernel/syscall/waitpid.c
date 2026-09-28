#include "syscall.h"
#include "scheduler.h"
#include "misc.h"

int sys_waitpid(cpu_task_state_t* state, uint32_t pid)
{
    await_pid(state, pid);
    return 1;
}
