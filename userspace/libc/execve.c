int execve(const char *pathname)
{
    int ret;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(11),
          "b"(pathname)
        : "memory"
    );

    return ret;
}
