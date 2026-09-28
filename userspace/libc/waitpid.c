#include "waitpid.h"

int waitpid(uint32_t pid)
{
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(7),
          "b"(pid)
        : "memory"
    );
    return ret;
}
