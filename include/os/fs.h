#ifndef _FS_H
#define _FS_H

#include "type.h"

#define MAGIC_NUMER 0x666
#define BLOCK_SIZE 4096
#define MAX_NAME_LEN 24
#define FS_START (128 * 1024 * 1024)
#define BLK_BIT_MAP_START (FS_START + 512)
#define INODE_BIT_MAP_SIZE 1024
#define INODE_MAX_NUMBER (8 * INODE_BIT_MAP_SIZE)
#define DATA_BLOCK_BIT_MAP_SIZE (32 * 1024)
#define DATA_BLOCK_MAX_NUMBER (8 * DATA_BLOCK_BIT_MAP_SIZE)

typedef char (*data_blk_ptr)[4096];
typedef data_blk_ptr (*indirect_blk_ptr)[1024];
typedef indirect_blk_ptr (*double_indrect_blk_ptr)[1024];
typedef double_indrect_blk_ptr (*triple_indirect_blk_ptr)[1024];

#define O_RDWR 0b11
#define O_RD 0b10
#define O_WR 0b01

#define SOFT_LINK 0b00
#define HARD_LINK 0b11

extern char cwd_str[10 * MAX_NAME_LEN];

enum entry_type
{
    notype,
    file,
    dir,
    soft_link
};

// 32B
typedef struct dentry_t
{
    uint32_t type;
    uint32_t inode_num;
    char name[MAX_NAME_LEN];
} dentry_t;

// 32B
typedef struct info_t
{
    char owner[16];
    char hash[16];
} info_t;

// 128B
typedef struct inode_t
{
    // attribute section

    uint32_t entry_num;
    uint32_t type;
    uint32_t size;
    uint32_t link_cnt; // a reference counter used in hard link
    uint32_t link_to;  // only valid in a soft link inode

    // data pointer section

    // The first direct data block of a directory inode is
    // used for storing dentries. './' and '../' are the
    // first two of them.
    data_blk_ptr direct[16];
    indirect_blk_ptr indirect;
    double_indrect_blk_ptr double_indrect;
    triple_indirect_blk_ptr triple_indrect;

    // useless part. maybe the only usage is for padding.

    info_t info;
} inode_t;

typedef struct super_block_t
{
    uint32_t magic;
    uint32_t data_block_num;
    uint32_t data_block_size;
    uint32_t inode_num;
    uint32_t inode_size;
    uint32_t blk_bit_map_start;
    uint32_t ind_bit_map_start;
    uint32_t ind_start;
    uint32_t data_start;
} super_block_t;

typedef struct file_desc_t
{
    uint32_t pos;
    inode_t *file;
    int file_access;
} file_desc_t;

extern inode_t *cwd;

inode_t *allocate_inode(uint32_t inode_number);
char *allocate_data_block(uint32_t data_block_number, char *buf);
uint32_t get_data_block_offset(uint32_t data_block_number);
uint32_t get_data_block_number(uint32_t data_block_offset);
uint32_t find_first_zero_bit(char *buf, uint32_t byte_sz);

void clear_bit(char *buf, uint32_t k);

uint32_t get_inode_number(inode_t *ind);

void free_inode(inode_t *ind);

void mount_fs();

void do_mkfs();

void do_fs_write(uint32_t offset, char *wbuf, uint32_t sz);

void do_fs_read(uint32_t offset, char *rbuf, uint32_t sz);

void do_print_fs_info();

void do_ls(char *name);

void do_mkdir(char *name);

void do_rm(char *name);

void do_cd(char *name);

void do_touch(char *name);

void do_cat(char *name);

void do_fopen(char *name, int access);

void do_fwrite(int fd, char *buf, uint32_t sz);

void do_fread(int fd, char *buf, uint32_t sz);

void do_fclose(int fd);

void do_fseek(int fd, uint32_t pos);

void do_link(char *_master, char *_servant, uint32_t link_type);

void do_find(char *path, char *name);

void do_rename(char *_old, char *_new);

void do_resize(inode_t *file, uint32_t sz);

#endif