#ifndef _IOCTL_H
#define _IOCTL_H

#define SET_FLAG_REQUEST   1
#define CLEAR_FLAG_REQUEST 2

#define CLEAR_SCREEN_FLAG (1 << 0)
#define ECHO_FLAG         (1 << 1)

// TODO: the third parameter of the ioctl on unix libc
// is actually ..., cause there are some requests that
// doesn't require arguments, so when ioctl is called
// for those it's not necessary to pass NULL as the last param.
int ioctl(int fd, unsigned long request, void *arg);

#endif
