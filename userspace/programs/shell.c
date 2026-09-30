#include "stdio.h"
#include "getcwd.h"
#include "ioctl.h"
#include "execvp.h"
#include "waitpid.h"
#include "chdir.h"

int main(void)
{
    unsigned long clear_flag;
    clear_flag |= CLEAR_SCREEN_FLAG;
    ioctl(stdin, SET_FLAG_REQUEST, (void*)clear_flag);

    unsigned long echo_flag;
    echo_flag |= ECHO_FLAG;
    ioctl(stdin, SET_FLAG_REQUEST, (void*)echo_flag);

    char command_buffer[100];
    char cwd_buffer[100];
    getcwd(cwd_buffer);
    int cursor = 0;
    printf("alphaOS$ %s> ", cwd_buffer);
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
            getcwd(cwd_buffer);
            printf("alphaOS$ %s> ", cwd_buffer);
            continue;
        }

        if (ch == '\b')
        {
            if (cursor > 0)
                cursor--;
            continue;
        }

        command_buffer[cursor] = ch;
        cursor++;
    }

    return 1;
}
