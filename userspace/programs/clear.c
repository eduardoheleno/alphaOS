#include "ioctl.h"
#include "stdio.h"

int main(void)
{
    unsigned long clear_flag;
    clear_flag |= CLEAR_SCREEN_FLAG;
    ioctl(stdin, SET_FLAG_REQUEST, (void*)clear_flag);
    return 1;
}
