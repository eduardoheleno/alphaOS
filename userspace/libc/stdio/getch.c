#include "stdio.h"
#include "unistd.h"

int getch(void)
{
    int ch;
    read(stdin, &ch, 1);
    return ch;
}
