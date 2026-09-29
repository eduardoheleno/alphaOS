#include "unistd.h"
#include "stdio.h"
#include "getdents.h"

int main(void)
{
    int fd = open(".");
    if (fd < 0)
    {
        printf("error\n");
    }
    else
    {
        struct dir_entry buffer[20];
        int entries_size = getdents(fd, buffer);
        for (int i = 0; i < entries_size; i++)
        {
            printf("%s\n", buffer[i].name);
        }
    }

    return 1;
}
