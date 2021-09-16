#include "fs.h"
#include "stdio.h"
#include "string.h"
#include "sched.h"
#include "algo.h"
#include "time.h"

char block_bit_map[DATA_BLOCK_BIT_MAP_SIZE]; // 32KB, 1024*256 bit
char inode_bit_map[INODE_BIT_MAP_SIZE];      // 1KB, 1024*8bit
inode_t nodes[INODE_MAX_NUMBER];
uint8_t inode_dirty[INODE_MAX_NUMBER / (BLOCK_SIZE / sizeof(inode_t))];

inode_t *cwd = nodes;
inode_t *root = nodes;

char cwd_str[10 * MAX_NAME_LEN];

super_block_t super_block;

// print the absolute path of a directory
static void print_absolute_path(inode_t *ind)
{
    char buf[BLOCK_SIZE];
    char str[10][MAX_NAME_LEN];
    dentry_t *dentry_buf = buf;
    uint32_t cur_inode_num = get_inode_number(ind);
    do_fs_read(ind->direct[0], buf, BLOCK_SIZE);
    uint32_t parent_inode_num;
    uint32_t cnt = 0;
    while (1)
    {
        parent_inode_num = dentry_buf[1].inode_num;
        if (parent_inode_num == cur_inode_num)
        {
            break;
        }
        inode_t *parent_inode = &nodes[parent_inode_num];
        do_fs_read(parent_inode->direct[0], buf, BLOCK_SIZE);
        for (int i = 0; i < parent_inode->entry_num; ++i)
        {
            if (dentry_buf[i].inode_num == cur_inode_num)
            {
                strcpy((char *)str[cnt], dentry_buf[i].name);
                cnt += 1;
                cur_inode_num = parent_inode_num;
                break;
            }
        }
    }
    printks("/");
    for (int i = cnt - 1; i >= 0; --i)
    {
        printks("%s/", (char *)str[i]);
    }
    printks("\n");
}

static void name_str_standrize(char *name)
{
    if (name == NULL)
    {
        return;
    }
    uint32_t len = strlen(name);
    if (name[len - 1] == '/')
    {
        name[--len] = '\0';
    }
}

static void mark_inode_dirty(uint32_t inode_number)
{
    uint32_t node_num_per_block = (BLOCK_SIZE + 16) / sizeof(inode_t);
    inode_dirty[inode_number / node_num_per_block] = 1;
}

static void update_inode()
{
    uint32_t node_num_per_block = (BLOCK_SIZE + 16) / sizeof(inode_t);
    uint32_t dirty_upper = INODE_MAX_NUMBER / (BLOCK_SIZE / sizeof(inode_t));
    for (int i = 0; i < dirty_upper; ++i)
    {
        if (inode_dirty[i])
        {
            inode_dirty[i] = 0;
            uint32_t media_offset = super_block.ind_start + sizeof(inode_t) * i * node_num_per_block;
            char *wrt_st = (uint32_t)nodes + sizeof(inode_t) * i * node_num_per_block;
            do_fs_write(media_offset, wrt_st, BLOCK_SIZE);
        }
    }
}

static void update_bit_map()
{
    do_fs_write(super_block.ind_bit_map_start, inode_bit_map, sizeof(inode_bit_map));
    do_fs_write(super_block.blk_bit_map_start, block_bit_map, sizeof(block_bit_map));
}

static void update_cwd_str(char *name, char *buf)
{
    inode_t *target;
    dentry_t *dentry_buf = buf;
    uint32_t cwd_str_len = strlen(cwd_str);

    if (name == NULL)
    {
        printks("[ERROR] invalid directory name.");
    }
    else
    {
        char __name[10 * MAX_NAME_LEN];
        char *_name = __name;
        strcpy(_name, name);
        if (_name[0] == '/')
        {
            // absolute path
            strcpy((char *)cwd_str, _name);
            cwd_str_len = strlen(cwd_str);
            cwd_str[cwd_str_len++] = '/';
            cwd_str[cwd_str_len] = '\0';
            return;
        }
        else
        {
            // relative path
            target = cwd;
        }

        int stop = 0;
        while (!stop)
        {
            do_fs_read(target->direct[0], buf, BLOCK_SIZE);
            int len;
            for (len = 0;; ++len)
            {
                if (_name[len] == '\0')
                {
                    stop = 1;
                    break;
                }
                else if (_name[len] == '/')
                {
                    _name[len] = '\0';
                    break;
                }
            }
            int found = 0;
            uint32_t _up = target->entry_num;
            for (int i = 0; i < _up; ++i)
            {
                if (strcmp(_name, dentry_buf[i].name) == 0)
                {
                    if (strcmp(_name, ".") == 0)
                    {
                        // nothing
                    }
                    else if (strcmp(_name, "..") == 0)
                    {
                        // goto parent dir
                        cwd_str[cwd_str_len - 1] = '\0';
                        while (cwd_str[cwd_str_len] != '/')
                        {
                            --cwd_str_len;
                        }
                        cwd_str[cwd_str_len++] = '/';
                        cwd_str[cwd_str_len] = '\0';
                    }
                    else
                    {
                        strcpy(&cwd_str[cwd_str_len], _name);
                        uint32_t delta = strlen(_name);
                        cwd_str_len += delta;
                        cwd_str[cwd_str_len++] = '/';
                        cwd_str[cwd_str_len] = '\0';
                    }
                    target = &nodes[dentry_buf[i].inode_num];
                    _name = &_name[len + 1];
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                stop = 1;
                target = NULL;
                break;
            }
        }
    }
}

void do_fs_write(uint32_t offset, char *wbuf, uint32_t sz)
{
    sdwrite(wbuf, offset, sz);
}

void do_fs_read(uint32_t offset, char *rbuf, uint32_t sz)
{
    sdread(rbuf, offset, sz);
}

static inode_t *name_parse(char *name, char *buf)
{
    inode_t *target;
    dentry_t *dentry_buf = buf;

    if (name == NULL)
    {
        target = cwd;
    }
    else
    {
        char __name[10 * MAX_NAME_LEN];
        char *_name = __name;
        strcpy(_name, name);
        if (_name[0] == '/')
        {
            // absolute path
            target = root;
            _name++;
        }
        else
        {
            // relative path
            target = cwd;
        }

        int stop = 0;
        while (!stop)
        {
            do_fs_read(target->direct[0], buf, BLOCK_SIZE);
            int len;
            for (len = 0;; ++len)
            {
                if (_name[len] == '\0')
                {
                    stop = 1;
                    break;
                }
                else if (_name[len] == '/')
                {
                    _name[len] = '\0';
                    break;
                }
            }
            int found = 0;
            uint32_t _up = target->entry_num;
            for (int i = 0; i < _up; ++i)
            {
                if (strcmp(_name, dentry_buf[i].name) == 0)
                {
                    target = &nodes[dentry_buf[i].inode_num];
                    _name = &_name[len + 1];
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                stop = 1;
                target = NULL;
                break;
            }
        }
    }
    if (target != NULL && target->type == soft_link)
    {
        int hash_check_pass = 1;
        for (int i = 0; i < 16; ++i)
        {
            if (nodes[target->link_to].info.hash[i] != target->info.hash[i])
            {
                hash_check_pass = 0;
                break;
            }
        }
        if (hash_check_pass)
        {
            target = &nodes[target->link_to];
        }
        else
        {
            printks("Invalid soft link!\n");
            target = NULL;
        }
    }
    return target;
}

void do_mkfs()
{
    super_block.magic = MAGIC_NUMER;
    super_block.data_block_num = DATA_BLOCK_MAX_NUMBER;
    super_block.data_block_size = BLOCK_SIZE;
    super_block.inode_num = INODE_MAX_NUMBER;
    super_block.inode_size = sizeof(inode_t);
    super_block.blk_bit_map_start = BLK_BIT_MAP_START;
    super_block.ind_bit_map_start = BLK_BIT_MAP_START + sizeof(block_bit_map);
    super_block.ind_start = super_block.ind_bit_map_start + sizeof(inode_bit_map);
    super_block.data_start = super_block.ind_start + sizeof(inode_t) * INODE_MAX_NUMBER;

    char buf[BLOCK_SIZE];
    memcpy(buf, &super_block, sizeof(super_block_t));
    do_fs_write(FS_START, buf, 512);
    memset(block_bit_map, 0, sizeof(block_bit_map));
    memset(inode_bit_map, 0, sizeof(inode_bit_map));

    // init root dir
    memset(buf, 0, BLOCK_SIZE);
    uint32_t root_inode_num = find_first_zero_bit(inode_bit_map, INODE_BIT_MAP_SIZE);
    root = allocate_inode(root_inode_num);
    root->entry_num = 2; // "./" and "../"
    root->link_cnt = 1;
    uint32_t root_data_block_number = find_first_zero_bit(block_bit_map, DATA_BLOCK_BIT_MAP_SIZE);
    dentry_t *dentry_buf = allocate_data_block(root_data_block_number, buf);
    root->direct[0] = get_data_block_offset(root_data_block_number);
    // "./"
    dentry_t *self = &dentry_buf[0];
    self->type = dir;
    self->inode_num = root_inode_num;
    strcpy(self->name, ".");
    // "../"
    dentry_t *parent = &dentry_buf[1];
    parent->type = dir;
    parent->inode_num = root_inode_num;
    strcpy(parent->name, "..");

    do_fs_write((uint32_t)root->direct[0], (char *)dentry_buf, BLOCK_SIZE);
    update_bit_map();
    mark_inode_dirty(root_inode_num);
    update_inode();
    root = nodes;
    cwd = nodes;
    strcpy(cwd_str, "/");

    do_print_fs_info();
}

void do_print_fs_info()
{
    char buf[512];
    printks("Reading FS meta data...\n");
    do_fs_read(FS_START, buf, 512);
    super_block_t *spblk = buf;
    if (spblk->magic != MAGIC_NUMER)
    {
        printks("No valid FS here.\n");
        return;
    }
    printks("\tMagic number: 0x%x\n", spblk->magic);
    printks("\tData block number: 0x%x\n", spblk->data_block_num);
    printks("\tData block size: 0x%x\n", spblk->data_block_size);
    printks("\tInode number: 0x%x\n", spblk->inode_num);
    printks("\tInode size: 0x%x\n", spblk->inode_size);
}

inode_t *allocate_inode(uint32_t inode_number)
{
    if (inode_number >= INODE_MAX_NUMBER)
    {
        return NULL;
    }
    uint32_t b = inode_number >> 3;
    uint32_t r = inode_number & 0b111;
    if (inode_bit_map[b] & (1 << r))
    {
        // bit map shows that inode_number is occupied
        return NULL;
    }
    else
    {
        inode_bit_map[b] |= (1 << r);
        for (int i = 0; i < 4; ++i)
        {
            uint32_t time = get_timer();
            char *tmp = &time;
            for (int j = 0; j < 4; ++j)
            {
                nodes[inode_number].info.hash[4 * i + j] = tmp[j];
            }
        }
        return &nodes[inode_number];
    }
}

char *allocate_data_block(uint32_t data_block_number, char *buf)
{
    if (data_block_number >= DATA_BLOCK_MAX_NUMBER)
    {
        return NULL;
    }
    uint32_t b = data_block_number >> 3;
    uint32_t r = data_block_number & 0b111;
    if (block_bit_map[b] & (1 << r))
    {
        // occupied
        return NULL;
    }
    else
    {
        block_bit_map[b] |= (1 << r);
        return buf;
    }
}

uint32_t find_first_zero_bit(char *buf, uint32_t byte_sz)
{
    for (uint32_t i = 0; i < 8 * byte_sz; ++i)
    {
        uint32_t b = i >> 3;
        uint32_t r = i & 0b111;
        if ((buf[b] & (1 << r)) == 0)
        {
            return i;
        }
    }
}

uint32_t get_data_block_offset(uint32_t data_block_number)
{
    uint32_t base = super_block.data_start;
    return base + data_block_number * BLOCK_SIZE;
}

void do_ls(char *name)
{
    if (name != NULL)
    {
        name_str_standrize(name);
    }
    inode_t *target;
    char buf[BLOCK_SIZE];
    dentry_t *dentry_buf = buf;
    target = name_parse(name, buf);
    if (target == NULL)
    {
        printks("No directory named %s\n", name);
        return;
    }

    do_fs_read(target->direct[0], buf, BLOCK_SIZE);
    uint32_t _up = target->entry_num;
    printks("Name        Type        Size\n");
    for (int i = 0; dentry_buf[i].type != notype && i < _up; ++i)
    {
        // name
        int met_zero = 0;
        for (int j = 0; j < 12; ++j)
        {

            if (dentry_buf[i].name[j] == '\0')
            {
                met_zero = 1;
            }
            char _char = met_zero ? ' ' : dentry_buf[i].name[j];
            printks("%c", _char);
        }
        // type
        if (dentry_buf[i].type == file)
        {
            printks("file        ");
        }
        else if (dentry_buf[i].type == dir)
        {
            printks("directory   ");
        }
        else if (dentry_buf[i].type == soft_link)
        {
            printks("soft link   ");
        }
        // size
        if (dentry_buf[i].type == file)
        {
            printks("%d\n", nodes[dentry_buf[i].inode_num].size);
        }
        else if (dentry_buf[i].type == dir)
        {
            printks("\n");
        }
        else if (dentry_buf[i].type == soft_link)
        {
            printks("\n");
        }
    }
}

uint32_t get_inode_number(inode_t *ind)
{
    return ((uint32_t)ind - (uint32_t)nodes) / sizeof(inode_t);
}

void do_mkdir(char *name)
{
    if (name == NULL)
    {
        printks("[ERROR] Invalid directory name.\n");
        return;
    }
    name_str_standrize(name);

    char _cwd_dentry_buf[BLOCK_SIZE];
    char _new_dir_dentry_buf[BLOCK_SIZE];
    dentry_t *cwd_dentry_buf = _cwd_dentry_buf;
    dentry_t *new_dir_dentry_buf = _new_dir_dentry_buf;

    do_fs_read(cwd->direct[0], _cwd_dentry_buf, BLOCK_SIZE);
    uint32_t cnt = cwd->entry_num++;
    cwd_dentry_buf[cnt].type = dir;
    uint32_t new_dir_inode_number = find_first_zero_bit(inode_bit_map, sizeof(inode_bit_map));
    inode_t *new_dir_inode = allocate_inode(new_dir_inode_number);
    cwd_dentry_buf[cnt].inode_num = new_dir_inode_number;
    strcpy(cwd_dentry_buf[cnt].name, name);
    do_fs_write(cwd->direct[0], _cwd_dentry_buf, BLOCK_SIZE);

    uint32_t cwd_inode_number = get_inode_number(cwd);
    uint32_t new_dir_data_block_number = find_first_zero_bit(block_bit_map, sizeof(block_bit_map));
    new_dir_dentry_buf = allocate_data_block(new_dir_data_block_number, _new_dir_dentry_buf);
    new_dir_dentry_buf[0].type = new_dir_dentry_buf[1].type = dir;
    new_dir_dentry_buf[0].inode_num = new_dir_inode_number;
    new_dir_dentry_buf[1].inode_num = cwd_inode_number;
    strcpy(new_dir_dentry_buf[0].name, ".");
    strcpy(new_dir_dentry_buf[1].name, "..");
    new_dir_inode->entry_num = 2;
    new_dir_inode->type = dir;
    new_dir_inode->direct[0] = super_block.data_start + new_dir_data_block_number * BLOCK_SIZE;
    new_dir_inode->link_cnt = 1;
    do_fs_write(new_dir_inode->direct[0], _new_dir_dentry_buf, BLOCK_SIZE);
    update_bit_map();
    mark_inode_dirty(cwd_inode_number);
    mark_inode_dirty(new_dir_inode_number);
    update_inode();
}

void mount_fs()
{
    char buf[512];
    printks("Reading FS meta data...\n");
    do_fs_read(FS_START, buf, 512);
    super_block_t *spblk = buf;
    if (spblk->magic != MAGIC_NUMER)
    {
        printk("No valid FS.\n");
        do_mkfs();
    }
    super_block = *spblk;
    do_fs_read(super_block.ind_bit_map_start, inode_bit_map, sizeof(inode_bit_map));
    do_fs_read(super_block.blk_bit_map_start, block_bit_map, sizeof(block_bit_map));
    do_fs_read(super_block.ind_start, nodes, sizeof(nodes));
    root = nodes;
    cwd = nodes;
    strcpy(cwd_str, "/");
}

static void rm_recurse(inode_t *ind)
{
    ind->link_cnt -= 1;
    if (ind->link_cnt > 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            uint32_t time = get_timer();
            char *tmp = &time;
            for (int j = 0; j < 4; ++j)
            {
                ind->info.hash[4 * i + j] = tmp[j];
            }
        }
        return;
    }
    if (ind->type == dir)
    {
        char buf[BLOCK_SIZE];
        dentry_t *dentry_buf = buf;
        do_fs_read(ind->direct[0], buf, BLOCK_SIZE);
        uint32_t cnt = ind->entry_num;
        for (int i = 2; i < cnt; ++i)
        {
            inode_t *sub_inode = &nodes[dentry_buf[i].inode_num];
            rm_recurse(sub_inode);
        }
        uint32_t k = get_data_block_number(ind->direct[0]);
        clear_bit(block_bit_map, k);
    }
    else if (ind->type == file)
    {
        // direct
        for (int i = 0; i < 16; ++i)
        {
            uint32_t k = get_data_block_number(ind->direct[i]);
            clear_bit(block_bit_map, k);
        }
    }
    free_inode(ind);
}

void do_rm(char *name)
{
    inode_t *to_remove = NULL;
    char buf[BLOCK_SIZE];
    dentry_t *dentry_buf = buf;
    name_str_standrize(name);
    to_remove = name_parse(name, buf);
    // no target or broken soft link
    if (to_remove == NULL)
    {
        // search it in cwd
        printks("Only can remove a broken soft link in current working directory\n");
        do_fs_read(cwd->direct[0], buf, BLOCK_SIZE);
        for (int i = 2; i < cwd->entry_num; ++i)
        {
            if (0 == strcmp(name, dentry_buf[i].name))
            {
                to_remove = &nodes[dentry_buf[i].inode_num];
                break;
            }
        }
    }
    if (to_remove == NULL)
    {
        printks("No file or directory named %s\n", name);
        return;
    }
    rm_recurse(to_remove);
    // remove it from its parent
    int is_multi_level = 0;
    for (int i = 0; name[i]; ++i)
    {
        if (name[i] == '/')
        {
            is_multi_level += 1;
        }
    }
    if (is_multi_level == 1)
    {
        is_multi_level = name[0] == '/';
    }
    inode_t *to_remove_parent;
    char *to_remove_name = name;
    // single level
    if (!is_multi_level)
    {
        to_remove_parent = cwd;
    }

    // multi level
    else
    {
        int len = strlen(name);
        while (name[len] != '/')
        {
            --len;
        }
        name[len] = '\0';
        to_remove_parent = name_parse(name, buf);
        to_remove_name = &name[len + 1];
    }
    do_fs_read(to_remove_parent->direct[0], buf, BLOCK_SIZE);
    uint32_t th = 1000000;
    for (int i = 2; i < to_remove_parent->entry_num; ++i)
    {
        if (strcmp(to_remove_name, dentry_buf[i].name) == 0)
        {
            th = i;
            break;
        }
    }
    for (int i = th + 1; i < to_remove_parent->entry_num; ++i)
    {
        dentry_buf[i - 1] = dentry_buf[i];
    }

    to_remove_parent->entry_num -= 1;
    do_fs_write(to_remove_parent->direct[0], buf, BLOCK_SIZE);
    do_fs_write(super_block.ind_bit_map_start, inode_bit_map, sizeof(inode_bit_map));
    do_fs_write(super_block.blk_bit_map_start, block_bit_map, sizeof(block_bit_map));
    update_inode();
}

void free_inode(inode_t *ind)
{
    uint32_t inode_num = get_inode_number(ind);
    mark_inode_dirty(inode_num);
    ind->entry_num = 0;
    ind->size = 0;
    ind->link_cnt = 0;
    ind->type = notype;
    ind->indirect = ind->double_indrect = ind->triple_indrect = 0;
    for (int i = 0; i < 16; ++i)
    {
        ind->direct[i] = 0;
    }
    for (int i = 0; i < 16; ++i)
    {
        ind->info.hash[i] = 0;
    }
    clear_bit(inode_bit_map, inode_num);
}

void clear_bit(char *buf, uint32_t k)
{
    uint32_t b = k >> 3;
    uint32_t r = k & 0b111;
    buf[b] &= (~(1 << r));
}

uint32_t get_data_block_number(uint32_t data_block_offset)
{
    uint32_t delta = data_block_offset - super_block.data_start;
    return delta / BLOCK_SIZE;
}

void do_cd(char *name)
{
    if (name != NULL)
    {
        name_str_standrize(name);
    }
    char buf[BLOCK_SIZE];
    inode_t *target = name_parse(name, buf);
    if (target == NULL)
    {
        printks("No directory named %s\n", name);
        return;
    }
    update_cwd_str(name, buf);
    cwd = target;
}

// create an inode without any data block
void do_touch(char *name)
{
    if (name == NULL)
    {
        printks("[ERROR] No file name provided\n");
        return;
    }
    name_str_standrize(name);
    for (int i = 0; name[i]; ++i)
    {
        if (name[i] == '/')
        {
            printks("Command \"touch\" only supports relative path\n");
            return;
        }
    }
    // allocate a new inode
    uint32_t new_inode_number = find_first_zero_bit(inode_bit_map, sizeof(inode_bit_map));
    inode_t *new_inode = allocate_inode(new_inode_number);
    mark_inode_dirty(new_inode_number);
    new_inode->size = 0;
    new_inode->type = file;
    new_inode->link_cnt = 1;

    // update dentry of cwd
    char buf[BLOCK_SIZE];
    dentry_t *dentry_buf = buf;
    do_fs_read(cwd->direct[0], buf, BLOCK_SIZE);

    dentry_buf[cwd->entry_num].inode_num = new_inode_number;
    dentry_buf[cwd->entry_num].type = file;
    strcpy(dentry_buf[cwd->entry_num].name, name);
    cwd->entry_num += 1;
    mark_inode_dirty(get_inode_number(cwd));

    // write changes into disk
    do_fs_write(cwd->direct[0], buf, BLOCK_SIZE);
    update_inode();
    update_bit_map();
}

void do_cat(char *name)
{
    if (name == NULL)
    {
        printks("[ERROR] No file name provided\n");
        return;
    }
    name_str_standrize(name);
    char buf[BLOCK_SIZE];
    inode_t *target = name_parse(name, buf);
    if (target == NULL)
    {
        printks("No file named %s\n", name);
        return;
    }
    if (target->type == dir)
    {
        printks("Please specify a file name\n");
    }
    else if (target->type == file)
    {
        uint32_t sz = target->size;

        if (sz == 0)
        {
            return;
        }
        else
        {
            // only print the first block to shell
            do_fs_read(target->direct[0], buf, BLOCK_SIZE);
            for (int i = 0; i < sz && i < BLOCK_SIZE; ++i)
            {
                printks("%c", buf[i]);
            }
        }
        printks("\n");
    }
}

void do_fopen(char *name, int access)
{
    if (name == NULL)
    {
        printks("[ERROR] No file name provided\n");
        return;
    }
    name_str_standrize(name);
    char buf[BLOCK_SIZE];
    inode_t *target = name_parse(name, buf);
    if (target == NULL)
    {
        // create a new one
        do_touch(name);
        target = name_parse(name, buf);
    }
    if (target->type == dir)
    {
        printks("[ERROR] Cannot open a directory as file\n");
        return;
    }
    else if (target->type == file)
    {
        current_running->fd[0].file = target;
        current_running->fd[0].file_access = access;
        current_running->fd[0].pos = 0;
    }
}
char fwrite_wbuf[BLOCK_SIZE];
void do_fwrite(int fd, char *buf, uint32_t sz)
{
    uint32_t cnt = 0;
    char *wbuf = fwrite_wbuf;
    file_desc_t *desc = &current_running->fd[fd];
    uint32_t pos = current_running->fd[fd].pos;
    int resized = 0;
    if ((desc->file->size + BLOCK_SIZE - 1) / BLOCK_SIZE < (pos + sz + BLOCK_SIZE - 1) / BLOCK_SIZE)
    {
        do_resize(desc->file, pos + sz);
        resized = 1;
    }

    if (desc->file_access & O_WR == 0)
    {
        printks("No write permission\n");
        return;
    }

    if (sz > BLOCK_SIZE)
    {
        for (int st = 0; st < sz; st += BLOCK_SIZE)
        {
            do_fwrite(fd, buf + st, MIN(BLOCK_SIZE, sz - st));
        }
        return;
    }
    if (pos / BLOCK_SIZE != (pos + sz) / BLOCK_SIZE)
    {
        uint32_t fst_sz = BLOCK_SIZE - (pos & (BLOCK_SIZE - 1));
        do_fwrite(fd, buf, fst_sz);
        do_fwrite(fd, buf + fst_sz, sz - fst_sz);
        return;
    }

    // assume that write within a block
    uint32_t blk_offset;
    if (pos < 16 * BLOCK_SIZE)
    {
        blk_offset = desc->file->direct[pos / BLOCK_SIZE];
    }
    do_fs_read(blk_offset, wbuf, BLOCK_SIZE);

    uint32_t r = pos & (BLOCK_SIZE - 1);
    for (int i = 0; i < sz && i < BLOCK_SIZE - r; ++i)
    {
        wbuf[i + r] = buf[i];
    }
    desc->pos += sz;
    if (!resized)
    {
        desc->file->size += sz;
    }
    do_fs_write(blk_offset, wbuf, BLOCK_SIZE);
    mark_inode_dirty(get_inode_number(desc->file));
    update_bit_map();
    update_inode();
}

char fread_rbuf[BLOCK_SIZE];
void do_fread(int fd, char *buf, uint32_t sz)
{
    uint32_t pos = current_running->fd[fd].pos;
    file_desc_t *desc = &current_running->fd[fd];
    if (pos + sz > desc->file->size)
    {
        printks("[ERROR] Access violation\n");
        return;
    }
    if (sz > BLOCK_SIZE)
    {
        for (int st = 0; st < sz; st += BLOCK_SIZE)
        {
            do_fread(fd, buf + st, MIN(BLOCK_SIZE, sz - st));
        }
        return;
    }
    if (pos / BLOCK_SIZE != (pos + sz) / BLOCK_SIZE)
    {
        uint32_t fst_sz = BLOCK_SIZE - (pos & (BLOCK_SIZE - 1));
        do_fread(fd, buf, fst_sz);
        do_fread(fd, buf + fst_sz, sz - fst_sz);
        return;
    }

    char *rbuf = fread_rbuf;
    // assume that read range is in a block
    uint32_t blk_offset;
    if (pos < 16 * BLOCK_SIZE)
    {
        blk_offset = desc->file->direct[pos / BLOCK_SIZE];
    }
    do_fs_read(blk_offset, rbuf, BLOCK_SIZE);
    if (pos & (BLOCK_SIZE - 1))
    {
        uint32_t r = pos & (BLOCK_SIZE - 1);
        for (int i = 0; i < sz && i < (BLOCK_SIZE - r); ++i)
        {
            buf[i] = rbuf[i + r];
        }
    }
    else
    {
        for (int i = 0; i < sz && i < BLOCK_SIZE; ++i)
        {
            buf[i] = rbuf[i];
        }
    }
    desc->pos += sz;
}

void do_fclose(int fd)
{
    file_desc_t *desc = &current_running->fd[fd];
    desc->pos = 0;
    desc->file_access = NULL;
    desc->file_access = 0;
}

void do_fseek(int fd, uint32_t pos)
{
    current_running->fd[fd].pos = pos;
}

void do_link(char *_master, char *_servant, uint32_t link_type)
{
    name_str_standrize(_master);
    name_str_standrize(_servant);
    char _buf[BLOCK_SIZE];
    char *buf = _buf;
    inode_t *master = name_parse(_master, buf);
    uint32_t master_inode_num = get_inode_number(master);
    int servant_name_flag = 1;
    for (int i = 0; _servant[i]; ++i)
    {
        if (_servant[i] == '/')
        {
            servant_name_flag = 0;
            break;
        }
    }
    if (servant_name_flag == 0)
    {
        printks("Only can make link in cwd\n");
        return;
    }

    if (link_type == HARD_LINK)
    {
        dentry_t *dentry_buf = _buf;
        do_fs_read(cwd->direct[0], dentry_buf, BLOCK_SIZE);
        uint32_t entry_num = cwd->entry_num;
        dentry_buf[entry_num].type = master->type;
        dentry_buf[entry_num].inode_num = master_inode_num;
        strcpy(dentry_buf[entry_num].name, _servant);
        cwd->entry_num += 1;
        master->link_cnt += 1;
        mark_inode_dirty(master_inode_num);
        do_fs_write(cwd->direct[0], _buf, BLOCK_SIZE);
        update_inode();
    }
    else if (link_type == SOFT_LINK)
    {
        if (master->type == soft_link)
        {
            printks("Cannot create a softlink of a softlink\n");
            return;
        }
        uint32_t inode_num = find_first_zero_bit(inode_bit_map, sizeof(inode_bit_map));
        inode_t *inode = allocate_inode(inode_num);
        inode->type = soft_link;
        inode->link_to = get_inode_number(master);
        for (int i = 0; i < 16; ++i)
        {
            inode->info.hash[i] = master->info.hash[i];
        }
        dentry_t *dentry_buf = _buf;
        do_fs_read(cwd->direct[0], dentry_buf, BLOCK_SIZE);
        uint32_t entry_num = cwd->entry_num;
        dentry_buf[entry_num].type = inode->type;
        dentry_buf[entry_num].inode_num = inode_num;
        strcpy(dentry_buf[entry_num].name, _servant);
        cwd->entry_num += 1;
        mark_inode_dirty(master_inode_num);
        do_fs_write(cwd->direct[0], _buf, BLOCK_SIZE);
        update_inode();
    }
}

static void find_recurse(inode_t *path, char *name, char *buf)
{
    dentry_t *dentry_buf = buf;
    do_fs_read(path->direct[0], buf, BLOCK_SIZE);
    uint32_t sub_dir_num = 0;
    inode_t *sub_dir[100];
    uint32_t entry_num = path->entry_num;
    for (int i = 2; i < entry_num; ++i)
    {
        if (dentry_buf[i].type == dir)
        {
            sub_dir[sub_dir_num++] = &nodes[dentry_buf[i].inode_num];
        }
        else if (dentry_buf[i].type == file)
        {
            if (strcmp(dentry_buf[i].name, name) == 0)
            {
                print_absolute_path(path);
            }
        }
    }
    for (int i = 0; i < sub_dir_num; ++i)
    {
        find_recurse(sub_dir[i], name, buf);
    }
}

void do_find(char *path, char *name)
{
    name_str_standrize(path);
    char _buf[BLOCK_SIZE];
    char *buf = _buf;
    inode_t *dir = name_parse(path, buf);
    if (dir->type == file)
    {
        printks("Please specify a directory name\n");
        return;
    }
    find_recurse(dir, name, buf);
}

void do_rename(char *_old, char *_new)
{
    name_str_standrize(_old);
    name_str_standrize(_new);
    char buf[BLOCK_SIZE];
    dentry_t *dentry_buf = buf;
    do_fs_read(cwd->direct[0], buf, BLOCK_SIZE);
    uint32_t entry_num = cwd->entry_num;
    for (int i = 2; i < entry_num; ++i)
    {
        if (strcmp(dentry_buf[i].name, _old) == 0)
        {
            strcpy(dentry_buf[i].name, _new);
            break;
        }
    }
    do_fs_write(cwd->direct[0], buf, BLOCK_SIZE);
}

void do_resize(inode_t *file, uint32_t sz)
{
    uint32_t used_blk = (file->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint32_t all_blk = (sz + BLOCK_SIZE - 1) / BLOCK_SIZE;
    char buf[BLOCK_SIZE];
    if (all_blk > used_blk)
    {
        file->size = sz;
        if (all_blk < 16)
        {
            for (int i = used_blk; i < all_blk; ++i)
            {
                uint32_t blk_num = find_first_zero_bit(block_bit_map, sizeof(block_bit_map));
                allocate_data_block(blk_num, buf);
                file->direct[i] = get_data_block_offset(blk_num);
            }
        }
        update_bit_map();
        mark_inode_dirty(file);
        update_inode();
    }
}