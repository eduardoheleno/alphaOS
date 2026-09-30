#include "getcwd.h"

int getcwd(char* buffer)
{
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(183),
          "b"(buffer)
        : "memory"
    );

    return ret;
}
