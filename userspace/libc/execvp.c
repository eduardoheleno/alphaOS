#include "execvp.h"
#include "stdlib.h"
#include "string.h"

int execvp(const char* pathname)
{
    char* path = getenv("PATH");
    char execpath[100];
    memset(execpath, 0, sizeof(execpath));
    memcpy(execpath, path, strlen(path));
    memcpy(execpath + strlen(path), (void*)pathname, strlen(pathname));

    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(11),
          "b"(execpath)
        : "memory"
    );

    return ret;
}
