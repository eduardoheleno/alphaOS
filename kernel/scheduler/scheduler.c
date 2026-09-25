#include "scheduler.h"

#include "memory.h"
#include "pic.h"
#include "timer.h"
#include "gdt.h"
#include "misc.h"
#include "tty.h"

task_t *current_task = NULL;

static task_t *idle_task = NULL;
static task_t *reaper_task = NULL;
static task_t *dead_queue = NULL;
task_t *awaiting_stdin = NULL;

static uint64_t task_total = 0;
static uint64_t next_pid = 0;

extern tss_t tss;
extern uintptr_t kernel_page_directory[];
extern void restore_task_context(cpu_task_state_t*);

static void wake_idle_task(void)
{
    idle_task->status = TASK_SCHEDULER_IDLE;
}

static void sleep_idle_task(void)
{
    idle_task->status = TASK_SLEEP;
}

static void wake_reaper_task(void)
{
    reaper_task->status = TASK_READY;
}

static void sleep_reaper_task(void)
{
    reaper_task->status = TASK_SLEEP;
}

static task_t* next_task(void)
{
    task_t *tmp_task = current_task->next;
    while (tmp_task->status != TASK_READY && tmp_task->status != TASK_SCHEDULER_IDLE)
    {
        tmp_task = tmp_task->next;
    }

    return tmp_task;
}

static void load_context(cpu_task_state_t* state)
{
    if (state != NULL)
    {
        current_task->context = *state;
        if (current_task->context.cs == USER_CS)
        {
            current_task->context.esp = state->useresp;
        }
        else
        {
            current_task->context.esp = (uint32_t)&state->useresp;
        }
    }
    task_t *ntask = next_task();
    current_task = ntask;
    if (current_task->pid != IDLE_PID)
        current_task->status = TASK_RUNNING;

    reset_quantum();
    tss.esp0 = (uint32_t)current_task->ring0_stack_base + PAGE_SIZE;
    load_cr3(current_task->cr3);
}

void await_stdin(cpu_task_state_t *state)
{
    disable_interrupts();

    current_task->status = TASK_SLEEP;
    state->eip -= 2;
    if (task_total == 1) wake_idle_task();
    awaiting_stdin = current_task;
    load_context(state);
    restore_task_context(&current_task->context);
}

void wake_stdin_task(void)
{
    if (awaiting_stdin != NULL)
    {
        awaiting_stdin->status = TASK_READY;
        awaiting_stdin = NULL;
    }
}

static void push_dead_queue(task_t *task)
{
    task->next = NULL;
    if (dead_queue == NULL)
    {
        dead_queue = task;
    }
    else
    {
        task_t *tmp_task = dead_queue;
        while (tmp_task->next != NULL)
        {
            tmp_task = tmp_task->next;
        }

        tmp_task->next = task;
    }
}

static task_t* pop_dead_queue(void)
{
    if (dead_queue == NULL) return NULL;
    task_t *popped_task = dead_queue;
    dead_queue = dead_queue->next;

    return popped_task;
}

void task_exit(void)
{
    disable_interrupts();

    if (--task_total == 0)
    {
        wake_idle_task();
    }

    task_t *tmp_task = current_task;
    while (tmp_task->next != current_task)
    {
        tmp_task = tmp_task->next;
    }
    tmp_task->next = current_task->next;

    task_t *dead_task = current_task;
    load_context(NULL);
    push_dead_queue(dead_task);
    wake_reaper_task();
    restore_task_context(&current_task->context);
}

static void task_trampoline(void)
{
    current_task->entry();
    task_exit();
}

task_t* create_ring3_task(char *path)
{
    task_t *new_task = kmalloc(sizeof(task_t));
    new_task->context.edi = 0;
    new_task->context.esi = 0;
    new_task->context.ebp = 0;
    new_task->context.ebx = 0;
    new_task->context.edx = 0;
    new_task->context.ecx = 0;
    new_task->context.eax = 0;

    new_task->fds[FD_STDIN] = open_file("/dev/tty");
    new_task->fds[FD_STDOUT] = open_file("/dev/tty");
    new_task->fds[FD_STDERR] = open_file("/dev/tty");
    new_task->total_fds = 3;

    new_task->pid = next_pid++;
    new_task->status = TASK_READY;
    new_task->type = RING3_TASK;
    new_task->context.eflags = 0x202;

    void* kernel_stack = kmalloc(PAGE_SIZE);
    new_task->ring0_stack_base = kernel_stack;
    new_task->ring0_stack_size = PAGE_SIZE;

    new_task->context.cs = USER_CS;
    new_task->context.ds = USER_DS;

    uint8_t* program_buffer = NULL;
    size_t program_size = load_in_memory(path, &program_buffer);
    new_task->cr3 = mmap_ring3(program_buffer, program_size);
    new_task->ring3_stack_base = (void*)USER_STACK;
    new_task->ring3_stack_size = PAGE_SIZE;
    new_task->context.esp = USER_STACK + PAGE_SIZE - sizeof(uint32_t);
    new_task->context.eip = USER_CODE;

    return new_task;
}

static task_t* create_task(void *entry, task_type_t type)
{
    task_t *new_task = kmalloc(sizeof(task_t));
    new_task->context.edi = 0;
    new_task->context.esi = 0;
    new_task->context.ebp = 0;
    new_task->context.ebx = 0;
    new_task->context.edx = 0;
    new_task->context.ecx = 0;
    new_task->context.eax = 0;

    new_task->fds[FD_STDIN] = open_file("/dev/tty");
    new_task->fds[FD_STDOUT] = open_file("/dev/tty");
    new_task->fds[FD_STDERR] = open_file("/dev/tty");
    new_task->total_fds = 3;

    new_task->pid = next_pid++;
    new_task->status = TASK_READY;
    new_task->type = type;
    new_task->context.eflags = 0x202;

    void* kernel_stack = kmalloc(PAGE_SIZE);
    new_task->ring0_stack_base = kernel_stack;
    new_task->ring0_stack_size = PAGE_SIZE;

    switch (type)
    {
        case RING0_TASK:
            new_task->context.cs = KERNEL_CS;
            new_task->context.ds = KERNEL_DS;
            new_task->cr3 = (uintptr_t)kernel_page_directory - KERNEL_BASE;
            new_task->entry = entry;
            new_task->context.esp = (uintptr_t)kernel_stack + PAGE_SIZE;
            new_task->context.eip = (uint32_t)task_trampoline;
            break;
        case RING3_TASK:
            new_task->context.cs = USER_CS;
            new_task->context.ds = USER_DS;
            // new_task->cr3 = mmap_ring3();
            new_task->ring3_stack_base = (void*)USER_STACK;
            new_task->ring3_stack_size = PAGE_SIZE;
            new_task->context.esp = USER_STACK + PAGE_SIZE;
            new_task->context.eip = USER_CODE;
            break;
    }
    new_task->next = NULL;
    return new_task;
}

void enqueue_task(char* path)
{
    task_t* new_task = create_ring3_task(path);
    task_t* tmp_task = current_task;
    while (tmp_task->next != current_task)
    {
        tmp_task = tmp_task->next;
    }

    tmp_task->next = new_task;
    new_task->next = current_task;
    task_total++;
    sleep_idle_task();
}

static void idle_task_loop(void)
{
    for (;;)
    {
        halt();
    }
}

static void reaper_task_loop(void)
{
    for (;;)
    {
        disable_interrupts();
        while (dead_queue != NULL)
        {
            task_t *dead_task = pop_dead_queue();
            if (dead_task->type == RING3_TASK) unmmap_ring3(dead_task->cr3);

            for (size_t i = 0; i < dead_task->total_fds; i++)
            {
                // TODO: fix this
                kfree(dead_task->fds[i]);
            }

            kfree(dead_task->ring0_stack_base);
            kfree(dead_task);
        }

        sleep_reaper_task();
        load_context(NULL);
        restore_task_context(&current_task->context);
    }
}

void init_scheduler(void)
{
    idle_task = create_task(&idle_task_loop, RING0_TASK);
    idle_task->status = TASK_SCHEDULER_IDLE;

    reaper_task = create_task(&reaper_task_loop, RING0_TASK);
    reaper_task->status = TASK_SLEEP;

    idle_task->next = reaper_task;
    reaper_task->next = idle_task;

    current_task = idle_task;

    terminal_writestring("Scheduler initialized\n");
}

void scheduler_tick(cpu_task_state_t* state)
{
    if (task_total == 0)
    {
        wake_idle_task();
    }

    task_t *ntask = next_task();
    if (ntask == current_task)
    {
        pic_send_eoi(0);
        return;
    }

    if (current_task->status != TASK_SCHEDULER_IDLE)
    {
        current_task->status = TASK_READY;
    }

    pic_send_eoi(0);
    load_context(state);
    restore_task_context(&current_task->context);
}
