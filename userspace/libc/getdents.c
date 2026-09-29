#include "getdents.h"

int getdents(int fd, void* buffer)
{
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(141),
          "b"(fd),
          "c"(buffer)
        : "memory"
    );

    return ret;
}
