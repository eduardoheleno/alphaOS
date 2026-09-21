#include "stdio.h"
#include "ioctl.h"

int main(void)
{
    unsigned long flags;
    flags |= ECHO_FLAG;
    ioctl(stdin, SET_FLAG_REQUEST, (void*)flags);

    char command_buffer[100];
    int cursor = 0;
    printf("alphaOS$ ");
    while (1)
    {
        int ch = getch();

        if (ch == '\n')
        {
            cursor = 0;
            printf("Unknown command: %s\n", command_buffer);
            printf("alphaOS$ ");
            continue;
        }

        command_buffer[cursor] = ch;
        cursor++;
    }

    return 1;
}
