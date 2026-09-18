#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <stddef.h>
#include <stdint.h>
#include "multiboot.h"

#define FS_MAGIC 0x776

#define IBLOCK_OFFSET 25
#define DATA_OFFSET   65

#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_STDERR 2

#define FILE_TYPE 1
#define DIR_TYPE  2

#define TAR_FILE_TYPE 48
#define TAR_DIR_TYPE  53

#define TOTAL_DEVICES 10

typedef struct file file_t;
typedef struct file_ops file_ops_t;

struct inode
{
    uint8_t type;
    uint16_t size;
    uint16_t sectors;
    uint32_t sector[14];
};
typedef struct inode inode_t;

struct dir_entry
{
    uint32_t inode_number;
    char name[28];
};

struct imdir_entry
{
    uint32_t inode_number;
    uint8_t type;
    char name[28];
    size_t size;
    uint8_t* data;
};

struct im_fs
{
    inode_t* inode;
    uint32_t entries_count;
    struct imdir_entry entries[28];
};

struct im_fs_index_table
{
    char name[28];
    uint32_t index;
    struct im_fs_index_table* next;
};

struct file_ops
{
    int (*read)(file_t* f, void *buffer, size_t len);
    void (*write)(file_t* f, const void *buffer, size_t len);
    int (*ioctl)(file_t* f, unsigned long request, void *arg);
    int (*close)(file_t* f);
};

struct file
{
    char name[28];
    file_ops_t ops;
    inode_t inode;
    uint32_t off;
};

struct tar_header
{
	char file_path[100];
	char file_mode[8];
	char owner_user_id[8];
	char owner_group_id[8];
	char file_size[12];
	char file_mtime[12];
	char header_checksum[8];
	char file_type;
	char link_path[100];

	char padding[255];
};
typedef struct tar_header tar_header;

void init_fs(multiboot_info_t* mbi);
file_t* open_file(char* path);
size_t load_in_memory(char* path, uint8_t** program_buffer);

#endif
