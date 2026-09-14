#ifndef _KERNEL_SCHEDULER_H
#define _KERNEL_SCHEDULER_H

#include <stdint.h>
#include <stddef.h>

#include "filesystem/fs.h"

#define IDLE_PID   0
#define REAPER_PID 1

#define MAX_FD_PER_PROCESS 16

struct cpu_task_state
{
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
    uint32_t useresp;
    uint32_t userss;
};
typedef struct cpu_task_state cpu_task_state_t;

struct cpu_exception_state
{
    uint32_t exception_code;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t error_code, eip, cs, eflags;
    uint32_t useresp;
    uint32_t userss;
};
typedef struct cpu_exception_state cpu_exception_state_t;

typedef void (task_entry_t)(void);

enum task_status
{
    TASK_SCHEDULER_IDLE,
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEP,
};
typedef enum task_status task_status_t;

enum task_type
{
    RING0_TASK,
    RING3_TASK
};
typedef enum task_type task_type_t;

struct task
{
    uint64_t pid;
    cpu_task_state_t context;
    uintptr_t cr3;

    char* cwd;
    // uint32_t cwd;

    file_t *fds[MAX_FD_PER_PROCESS];
    size_t total_fds;

    void *ring0_stack_base;
    size_t ring0_stack_size;

    void *ring3_stack_base;
    size_t ring3_stack_size;

    task_entry_t *entry;
    task_status_t status;
    task_type_t type;
    struct task *next;
};
typedef struct task task_t;

void init_scheduler(void);
void enqueue_task(void *entry, task_type_t type);
void scheduler_tick(cpu_task_state_t *state);
void task_exit(void);
void await_stdin(cpu_task_state_t *state);
void wake_stdin_task(void);

#endif
