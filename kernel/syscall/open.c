#include "syscall.h"
#include "filesystem/fs.h"
#include "memory.h"
#include "tty.h"

int sys_open(const char* path)
{
    char fullpath[100];
    kmemset(fullpath, 0, sizeof(fullpath));
    kmemcpy(fullpath, current_task->cwd, strlen(current_task->cwd));
    kmemcpy(fullpath + strlen(current_task->cwd), (void*)path, strlen(path));
    file_t* file = open_file(fullpath);
    if (file == NULL)
        return -1;
    int fd = current_task->total_fds;
    current_task->fds[fd] = file;
    current_task->total_fds++;

    return fd;
}
