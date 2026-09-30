#include "syscall.h"

#include "tty.h"
#include "memory.h"
#include "filesystem/fs.h"

int sys_chdir(const char* path)
{
    char target_path[100];
    kmemcpy(target_path, current_task->cwd, strlen(current_task->cwd));
    kmemcpy(target_path + strlen(current_task->cwd), (void*)path, strlen(path));
    file_t* f = open_file(target_path);
    if (f == NULL)
        return -1;

    char* alloc_path = kmalloc(strlen(current_task->cwd) + strlen(path));
    kmemcpy(alloc_path, target_path, strlen(current_task->cwd) + strlen(path));
    kfree(current_task->cwd);
    current_task->cwd  = alloc_path;

    return 1;
}
