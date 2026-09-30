#include "chdir.h"

int chdir(const char* path)
{
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(12),
          "b"(path)
        : "memory"
    );

    return ret;
}
