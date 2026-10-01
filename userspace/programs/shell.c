#include "stdio.h"
#include "string.h"
#include "getcwd.h"
#include "ioctl.h"
#include "chdir.h"
#include "execvp.h"
#include "waitpid.h"

void build_command(const char* command_buffer, char* program_buffer, char* arg_buffer)
{
    int program_buffer_cursor = 0;
    int arg_buffer_cursor = 0;
    int has_arg_started = 0;
    for (int i = 0; i < (int)strlen(command_buffer); i++)
    {
        if (command_buffer[i] == ' ')
        {
            has_arg_started = 1;
            i++;
        }

        if (has_arg_started == 1)
        {
            arg_buffer[arg_buffer_cursor] = command_buffer[i];
            arg_buffer_cursor++;
        }
        else
        {
            program_buffer[program_buffer_cursor] = command_buffer[i];
            program_buffer_cursor++;
        }
    }
}

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
            char program_buffer[100];
            char arg_buffer[100];
            build_command(command_buffer, program_buffer, arg_buffer);

            int command_pid;
            // TODO: resolve "." and ".."
            if (strcmp(program_buffer, "cd", 2) == 0)
            {
                int return_code = chdir(arg_buffer);
                if (return_code < 0)
                    printf("The directory '%s' does not exist\n", arg_buffer);
                goto endflow;
            }
            else
            {
                command_pid = execvp(program_buffer, NULL);
            }

            if (command_pid > 0)
            {
                waitpid(command_pid);
            }
            else
            {
                printf("Unknown command: %s\n", command_buffer);
            }

endflow:
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
