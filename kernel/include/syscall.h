#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include <stdint.h>
#include <stddef.h>

#include "scheduler.h"

extern task_t *current_task;

int sys_read(cpu_task_state_t *state, uintptr_t fd, char *buffer, size_t len);
int sys_write(uintptr_t fd, const void *buffer, size_t len);
int sys_ioctl(uintptr_t fd, unsigned long request, void *arg);
int sys_mmap(void *addr, size_t len);
int sys_munmap(void* addr, size_t len);
int sys_execve(const char* path);

#endif
