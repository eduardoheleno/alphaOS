#include "filesystem/device.h"
#include "tty.h"
#include "memory.h"

static void register_device(const char* name, file_ops_t ops,
        file_t* buffer, uint32_t index)
{
    file_t device;
    kmemcpy(device.name, (void*)name, strlen(name));
    device.ops = ops;
    buffer[index] = device;
}

void init_devices(file_t* device_buffer)
{
    register_device("tty", tty_ops(), device_buffer, 0);
}
