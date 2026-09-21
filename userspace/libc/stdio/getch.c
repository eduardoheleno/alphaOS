#include "stdio.h"
#include "unistd.h"

int getch(void)
{
    unsigned char ch;
    if (read(stdin, &ch, 1) != 1) return -1;
    return ch;
}
