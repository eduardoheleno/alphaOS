#include "stdio.h"
#include "ioctl.h"
#include "execvp.h"
#include "waitpid.h"

int main(void)
{
    unsigned long clear_flag;
    clear_flag |= CLEAR_SCREEN_FLAG;
    ioctl(stdin, SET_FLAG_REQUEST, (void*)clear_flag);

    unsigned long echo_flag;
    echo_flag |= ECHO_FLAG;
    ioctl(stdin, SET_FLAG_REQUEST, (void*)echo_flag);

    char command_buffer[100];
    int cursor = 0;
    printf("alphaOS$ ");
    while (1)
    {
        int ch = getch();

        if (ch == '\n')
        {
            command_buffer[cursor] = '\0';
            int command_pid = execvp(command_buffer);
            if (command_pid > 0)
            {
                waitpid(command_pid);
            }
            else
            {
                printf("Unknown command: %s\n", command_buffer);
            }
            cursor = 0;
            printf("alphaOS$ ");
            continue;
        }

        command_buffer[cursor] = ch;
        cursor++;
    }

    return 1;
}
