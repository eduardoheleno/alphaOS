#include "filesystem/fs.h"
#include "filesystem/disk.h"
#include "memory.h"
#include "tty.h"
#include "misc.h"

// TODO: improve naming

uint32_t global_root_inode_num;

vnode_t *global_vfs_root = NULL;
vnode_t *global_tty = NULL;

static vnode_t* create_vnode(const char *name)
{
    vnode_t *vnode = kmalloc(sizeof(vnode_t));
    vnode->name = name;
    vnode->parent = NULL;
    vnode->children = NULL;
    vnode->next_sibling = NULL;
    vnode->ops = NULL;

    return vnode;
}

static void mount_file_on_dir(vnode_t *file, vnode_t *dir)
{
    file->parent = dir;
    if (dir->children == NULL)
    {
        dir->children = file;
    }
    else
    {
        vnode_t *tmp = dir->children;
        while (tmp->next_sibling)
        {
            tmp = tmp->next_sibling;
        }
        tmp->next_sibling = file;
    }
}

static void init_ibmap(void)
{
    uint8_t bitmap_area[4096];
    kmemset(bitmap_area, 0, sizeof(bitmap_area));
    disk_write(9, 8, bitmap_area);
}

static void init_dbmap(void)
{
    uint8_t bitmap_area[4096];
    kmemset(bitmap_area, 0, sizeof(bitmap_area));
    disk_write(17, 8, bitmap_area);
}

static void init_iblock(void)
{
    uint8_t* iblock_area = kmalloc(20480);
    kmemset(iblock_area, 0, 20480);
    disk_write(IBLOCK_OFFSET, 40, iblock_area);
    kfree(iblock_area);
}

static int alloc_inode_num(void)
{
    uint8_t inode_bmap[4096];
    kmemset(inode_bmap, 0, sizeof(inode_bmap));
    disk_read(9, 8, (uint16_t*)inode_bmap);

    for (uint16_t i = 0; i < 4096; i++)
    {
        for (uint16_t j = 0; j < 8; j++)
        {
            uint8_t bit = (inode_bmap[i] >> j) & 1;
            if (bit == 0)
            {
                inode_bmap[i] |= (1 << j);
                disk_write(9, 8, inode_bmap);
                return i * 8 + j;
            }
        }
    }

    return -1;
}

static void write_inode_data(inode_t* inode, uint8_t* data, size_t size)
{
    uint8_t data_bmap[4096];
    kmemset(data_bmap, 0, sizeof(data_bmap));
    disk_read(17, 8, (uint16_t*)data_bmap);

    uint8_t* data_buffer = kmalloc(512 * inode->sectors);
    kmemset(data_buffer, 0, 512 * inode->sectors);
    kmemcpy(data_buffer, data, size);
    uint32_t sectors_count = 0;
    for (uint16_t i = 0; i < 4096; i++)
    {
        for (uint16_t j = 0; j < 8; j++)
        {
            if (sectors_count >= inode->sectors) break;

            uint8_t bit = (data_bmap[i] >> j) & 1;
            if (bit == 0)
            {
                data_bmap[i] |= (1 << j);
                uint32_t sector_idx = i * 8 + j;
                inode->sector[sectors_count] = sector_idx;
                disk_write(DATA_OFFSET + sector_idx, 1, &data_buffer[sectors_count++ * 512]);
            }
        }
    }

    disk_write(17, 8, data_bmap);
    kfree(data_buffer);
}

static uint8_t* read_inode_data(inode_t inode)
{
    uint8_t* data_buffer = kmalloc(512 * inode.sectors);
    for (uint32_t i = 0; i < inode.sectors; i++)
    {
        disk_read(DATA_OFFSET + inode.sector[i], 1, (uint16_t*)&data_buffer[i * 512]);
    }

    return data_buffer;
}

static void write_inode(inode_t inode, uint32_t inode_num)
{
    uint32_t inode_sector = inode_num / 8;
    uint32_t inode_offset = inode_num % 8;
    uint8_t sector_buffer[512];
    disk_read(IBLOCK_OFFSET + inode_sector, 1, (uint16_t*)sector_buffer);

    kmemcpy(&sector_buffer[inode_offset * sizeof(inode_t)], &inode, sizeof(inode_t));
    disk_write(IBLOCK_OFFSET + inode_sector, 1, sector_buffer);
}

static void read_inode(uint32_t inode_num, inode_t* inode_buffer)
{
    uint32_t inode_sector = inode_num / 8;
    uint32_t inode_offset = inode_num % 8;
    uint8_t sector_buffer[512];
    disk_read(IBLOCK_OFFSET + inode_sector, 1, (uint16_t*)sector_buffer);

    kmemcpy(inode_buffer, &sector_buffer[inode_offset * sizeof(inode_t)], sizeof(inode_t));
}

static uint8_t tar_zero_block(const uint8_t *block)
{
    for (size_t i = 0; i < 512; i++) {
        if (block[i] != 0) {
            return 0;
        }
    }

    return 1;
}

static void add_dir_inode_im_fs(char* name, char* parent_name, int inode_num, int parent_num,
        struct im_fs* inmemory_fs, struct im_fs_index_table** head)
{
    struct im_fs_index_table* tmp = *head;
    inode_t* inode = kmalloc(sizeof(inode_t));
    inode->type = DIR_TYPE;

    if (tmp == NULL)
    {
        struct im_fs_index_table* index_entry = kmalloc(sizeof(struct im_fs_index_table));
        kmemcpy(index_entry->name, name, 28);
        index_entry->index = 0;
        index_entry->next = NULL;
        *head = index_entry;

        inmemory_fs[index_entry->index].inode = inode;
        inmemory_fs[index_entry->index].entries[0].inode_number = inode_num;
        inmemory_fs[index_entry->index].entries[1].inode_number = parent_num;
        kmemcpy(inmemory_fs[index_entry->index].entries[0].name, ".", 1);
        kmemcpy(inmemory_fs[index_entry->index].entries[1].name, "..", 2);
        inmemory_fs[index_entry->index].entries_count = 2;
        return;
    }

    uint32_t parent_index = 0;
    struct im_fs_index_table* buf = *head;
    while (buf != NULL)
    {
        if (kstrcmp(buf->name, parent_name, strlen(buf->name)) == 0)
        {
            parent_index = buf->index;
            break;
        }
        buf = buf->next;
    }

    while (tmp->next != NULL)
    {
        tmp = tmp->next;
    }

    struct im_fs_index_table* index_entry = kmalloc(sizeof(struct im_fs_index_table));
    kmemcpy(index_entry->name, name, 28);
    index_entry->index = tmp->index + 1;
    index_entry->next = NULL;
    tmp->next = index_entry;

    inmemory_fs[index_entry->index].inode = inode;
    inmemory_fs[index_entry->index].entries[0].inode_number = inode_num;
    inmemory_fs[index_entry->index].entries[1].inode_number = parent_num;
    kmemcpy(inmemory_fs[index_entry->index].entries[0].name, ".", 1);
    kmemcpy(inmemory_fs[index_entry->index].entries[1].name, "..", 2);
    inmemory_fs[index_entry->index].entries_count = 2;

    struct dir_entry* entries = inmemory_fs[parent_index].entries;
    uint32_t entries_count = inmemory_fs[parent_index].entries_count;

    kmemcpy(entries[entries_count++].name, name, 28);
    entries[entries_count].inode_number = inode_num;

    inmemory_fs[parent_index].entries_count = entries_count;
    inmemory_fs[parent_index].inode->size = sizeof(struct dir_entry) * entries_count;
    return;
}

static void init_im_fs(struct im_fs* inmemory_fs, struct im_fs_index_table** head)
{
    int inode_number = alloc_inode_num();
    add_dir_inode_im_fs("/", "/", inode_number, inode_number, inmemory_fs, head);
    global_root_inode_num = inode_number;
}

static int lookup_parent_inode_num(char* file_path, struct im_fs* inmemory_fs,
        struct im_fs_index_table* head)
{
    int first_slash = 0;
    for (int i = strlen(file_path) - 2; i >= 0; i--)
    {
        if (file_path[i] == '/')
        {
            if (first_slash == 0)
            {
                first_slash = i;
                if (first_slash == 1) return global_root_inode_num;
                continue;
            }

            char parent_name[28];
            kmemset(parent_name, 0, sizeof(parent_name));
            uint16_t cursor = 0;
            for (int j = i + 1; j < first_slash; j++)
            {
                parent_name[cursor++] = file_path[j];
            }

            while (head != NULL)
            {
                if (kstrcmp(head->name, parent_name, strlen(head->name)) == 0)
                {
                    return inmemory_fs[head->index].entries[0].inode_number;
                }
                head = head->next;
            }
        }
    }

    return -1;
}

static char* extract_dirname(char* file_path)
{
    char* dirname = kmalloc(28);
    for (int i = strlen(file_path) - 2; i >= 0; i--)
    {
        if (file_path[i] == '/')
        {
            uint16_t cursor = 0;
            for (int j = i + 1; j < (int)strlen(file_path) - 1; j++)
            {
                dirname[cursor++] = file_path[j];
            }

            return dirname;
        }
    }

    return NULL;
}

static char* extract_filename(char* file_path)
{
    char* dirname = kmalloc(28);
    for (int i = strlen(file_path) - 1; i >= 0; i--)
    {
        if (file_path[i] == '/')
        {
            uint16_t cursor = 0;
            for (int j = i + 1; j < (int)strlen(file_path); j++)
            {
                dirname[cursor++] = file_path[j];
            }

            return dirname;
        }
    }

    return NULL;
}

static char* extract_parent_name(char* file_path)
{
    char* parent_name = kmalloc(28);
    int first_slash = 0;
    for (int i = strlen(file_path) - 1; i >= 0; i--)
    {
        if (file_path[i] == '/')
        {
            if (first_slash == 0)
            {
                first_slash = i;
                continue;
            }

            uint16_t cursor = 0;
            for (int j = i + 1; j < first_slash; j++)
            {
                parent_name[cursor++] = file_path[j];
            }
            return parent_name;
        }
    }

    return NULL;
}

static char* extract_parent_name2(char* file_path)
{
    char* parent_name = kmalloc(28);
    int first_slash = 0;
    for (int i = strlen(file_path) - 2; i >= 0; i--)
    {
        if (file_path[i] == '/')
        {
            if (first_slash == 0)
            {
                first_slash = i;
                continue;
            }

            uint16_t cursor = 0;
            for (int j = i + 1; j < first_slash; j++)
            {
                parent_name[cursor++] = file_path[j];
            }
            return parent_name;
        }

        if (file_path[i] == '.')
        {
            parent_name = "/";
            return parent_name;
        }
    }

    return NULL;
}

static void add_file_inode_im_fs(char* name, char* parent_name, int inode_num,
        struct im_fs* inmemory_fs, struct im_fs_index_table** head)
{
    struct im_fs_index_table* tmp = *head;
    int index = 0;
    while (tmp != NULL)
    {
        if (kstrcmp(tmp->name, parent_name, 28) == 0)
        {
            index = tmp->index;
            break;
        }
        tmp = tmp->next;
    }

    struct dir_entry* entries = inmemory_fs[index].entries;
    uint32_t entries_count = inmemory_fs[index].entries_count;
    kmemcpy(entries[entries_count++].name, name, 28);
    entries[entries_count].inode_number = inode_num;

    inmemory_fs[index].entries_count = entries_count;
    inmemory_fs[index].inode->size = sizeof(struct dir_entry) * entries_count;
}

void init_fs(multiboot_info_t* mbi)
{
    // TODO: check possible error
    init_disk();
    init_ibmap();
    init_dbmap();
    init_iblock();

    struct im_fs inmemory_fs[28];
    struct im_fs_index_table* head = NULL;

    init_im_fs(inmemory_fs, &head);

    multiboot_module_t* mbm = (multiboot_module_t*)mbi->mods_addr;
    uint8_t* cursor = (uint8_t*)(uintptr_t)mbm->mod_start;
    uint8_t* end = (uint8_t*)(uintptr_t)mbm->mod_end;

    tar_header *first = (tar_header *)cursor;
    uint64_t first_size =
        tar_parse_octal(first->file_size, sizeof(first->file_size));

    cursor = (uint8_t *)first +
        512 +
        ((first_size + 511) & ~(uint64_t)511);

    while ((size_t)(end - cursor) >= 512) 
    {
        if (tar_zero_block(cursor)) 
        {
            if ((size_t)(end - cursor) >= 1024 &&
                tar_zero_block(cursor + 512)) 
            {
                break;
            }

            break;
        }

        tar_header *th = (tar_header *)cursor;
        uint64_t file_size = tar_parse_octal(th->file_size, sizeof(th->file_size));

        uint8_t *file_data = cursor + 512;
        uint64_t padded_size = (file_size + 511) & ~(uint64_t)511;

        if (padded_size > (uint64_t)(end - file_data)) 
        {
            break;
        }

        if (th->file_type == TAR_DIR_TYPE)
        {
            char* dir_name = extract_dirname(th->file_path);
            int inode_num = alloc_inode_num();
            int parent_num = lookup_parent_inode_num(th->file_path, inmemory_fs, head);
            char* parent_name = extract_parent_name2(th->file_path);
            // debug_write(parent_name);
            // debug_write("\n");
            // debug_write(th->file_path);
            // debug_write("\n");
            // debug_write("\n");
            add_dir_inode_im_fs(dir_name, parent_name, inode_num, parent_num, inmemory_fs, &head);
            kfree(dir_name);
            kfree(parent_name);
        }
        else if (th->file_type == TAR_FILE_TYPE)
        {
            int inode_num = alloc_inode_num();
            char* filename = extract_filename(th->file_path);
            char* parent_name = extract_parent_name(th->file_path);
            add_file_inode_im_fs(filename, parent_name, inode_num, inmemory_fs, &head);
            // debug_write(th->file_path);
            // debug_write("\n");
            // debug_write("filename: ");
            // debug_write(extract_filename(th->file_path));
            // debug_write("\n");
            // debug_write("parent_name: ");
            // debug_write(extract_parent_name(th->file_path));
            // debug_write("\n");
            // debug_int(lookup_parent_inode_num(th->file_path, inmemory_fs, head));
            // debug_write("\n");

            // debug_write("\n");
            // debug_write("\n");
        }
        // debug_write(th->file_path);
        // debug_write("\n");
        // debug_int(th->file_type);
        // debug_write("\n");
        // uint32_t parent_num = lookup_parent_inode_num(th->file_path);
        // debug_int(parent_num);
        // debug_int(strlen(th->file_path));
        // debug_write("\n");
        // debug_write("\n");
        cursor = file_data + padded_size;
    }

    // debug_int(head->index);
    // debug_write("\n");
    debug_write(inmemory_fs[3].entries[2].name);
    debug_write("\n");
    // while (cursor + 1024 <= end)
    // {
    //     if (tar_zero_block(cursor) &&)
    // }
    // tar_header* th = (tar_header*)mbm->mod_start;

    // uint64_t file_size = tar_parse_octal(th->file_size, sizeof(th->file_size));
    // uint8_t* file_data = (uint8_t*)th + sizeof(*th);

    // th = (tar_header *)(
    //     file_data + ((file_size + 511) & ~(uint64_t)511)
    // );

    // th = (tar_header *)(
    //     (uint8_t *)th +
    //     512 +
    //     ((tar_parse_octal(th->file_size, sizeof(th->file_size)) + 511) & ~511)
    // );
    // th = (tar_header *)(
    //     (uint8_t *)th +
    //     512 +
    //     ((tar_parse_octal(th->file_size, sizeof(th->file_size)) + 511) & ~511)
    // );
    // th = (tar_header *)(
    //     (uint8_t *)th +
    //     512 +
    //     ((tar_parse_octal(th->file_size, sizeof(th->file_size)) + 511) & ~511)
    // );
    // th = (tar_header *)(
    //     (uint8_t *)th +
    //     512 +
    //     ((tar_parse_octal(th->file_size, sizeof(th->file_size)) + 511) & ~511)
    // );
    // th = (tar_header *)(
    //     (uint8_t *)th +
    //     512 +
    //     ((tar_parse_octal(th->file_size, sizeof(th->file_size)) + 511) & ~511)
    // );
    // debug_write(th->file_path);
    // debug_write("\n");
    // debug_int(th->file_type);
    // file_data[0] -> file_data[file_size - 1]
    



    // th = (tar_header *)(
    //     (uint8_t *)th +
    //     512 +
    //     ((tar_parse_octal(th->file_size, sizeof(th->file_size)) + 511) & ~511)
    // );
    // th = (tar_header *)(
    //     (uint8_t *)th +
    //     512 +
    //     ((tar_parse_octal(th->file_size, sizeof(th->file_size)) + 511) & ~511)
    // );
    // th = (tar_header *)(
    //     (uint8_t *)th +
    //     512 +
    //     ((tar_parse_octal(th->file_size, sizeof(th->file_size)) + 511) & ~511)
    // );
    // th = (tar_header *)(
    //     (uint8_t *)th +
    //     512 +
    //     ((tar_parse_octal(th->file_size, sizeof(th->file_size)) + 511) & ~511)
    // );
    // debug_write(th->file_path);

    // int iroot_num = alloc_inode_num();
    // struct dir_entry root_entries[2];
    // root_entries[0].inode_number = iroot_num;
    // root_entries[1].inode_number = iroot_num;
    // kmemcpy(root_entries[0].name, ".", 1);
    // kmemcpy(root_entries[1].name, "..", 2);
    //
    // inode_t root_inode;
    // root_inode.type = DIR_TYPE;
    // root_inode.size = sizeof(root_entries);
    // root_inode.sectors = (sizeof(root_entries) + 511) / 512;
    //
    // write_inode_data(&root_inode, (uint8_t*)root_entries, sizeof(root_entries));
    // write_inode(root_inode, iroot_num);
    //
    // read_inode_data(root_inode);
}

void init_vfs(multiboot_info_t* mbi)
{
    multiboot_module_t* mbm = (multiboot_module_t*)mbi->mods_addr;
    tar_header* th = (tar_header*)mbm->mod_start;
    // th = (tar_header*)((uint32_t)th + 512);

    debug_write(th->file_path);
    // debug_hex32((uint32_t)th);
    // debug_write("\n");
    // debug_hex32(mbm->mod_start + 512);

    global_vfs_root = create_vnode("/");
    vnode_t *dev_dir = create_vnode("dev");
    global_tty = create_vnode("tty");

    global_tty->ops = tty_ops();

    mount_file_on_dir(global_tty, dev_dir);
    mount_file_on_dir(dev_dir, global_vfs_root);
}

file_t* open_file(vnode_t *vnode, uint8_t flags)
{
    file_t *file = kmalloc(sizeof(file_t));
    file->name = vnode->name;
    file->ops = vnode->ops;
    file->flags = flags;

    return file;
}
