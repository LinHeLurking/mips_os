#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512
#define OS_SIZE_LOC 2
#define BOOT_LOADER_SIG_OFFSET 0x1fe
#define BOOT_LOADER_SIG_1 0x55
#define BOOT_LOADER_SIG_2 0xaa
#define BOOT_MEM_LOC 0x7c00
#define OS_MEM_LOC 0x1000

/* structure to store command line options */
static struct
{
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void extent_opt(Elf32_Phdr *Phdr_bb, Elf32_Phdr *Phdr_k);
static void record_kernel_sectors(FILE *image, int kernelsz);
static void write_kernel(FILE *image, FILE *knfile, Elf32_Phdr *Phdr, int kernelsz);
static void write_bootblock(FILE *image, FILE *file, Elf32_Phdr *phdr);
static uint32_t count_kernel_sectors(Elf32_Phdr *Phdr);
static Elf32_Phdr *read_exec_file(FILE *opfile);
static void create_image(int nfiles, char *files[]);

/* garbages 


static Elf32_Ehdr *read_ehdr(FILE *opfile);
static void read_phdr(Elf32_Phdr *phdr, FILE *fp, int ph,
                      Elf32_Ehdr ehdr);
static void write_segment(Elf32_Ehdr ehdr, Elf32_Phdr phdr, FILE *fp,
                          FILE *img, int *nbytes, int *first);
static void write_os_size(int nbytes, FILE *img);
*/

/* assistant functions */
static void print_usage();
static void error(char *fmt, ...);

int main(int argc, char **argv)
{
    char *btblk_name, *kernel_name;
    if (argc < 3 || argc > 5)
    {
        print_usage();
        goto bad_para;
    }
    else if (argc == 3)
    {
        // ./createimage bootblock main
        btblk_name = argv[1];
        kernel_name = argv[2];
    }
    else if (argc == 4)
    {
        // ./createimage --extended bootblock main
        // ./createimage --vm bootblock main
        if (strcmp(argv[1], "--extended") == 0)
        {
            options.extended = 1;
        }
        else if (strcmp(argv[1], "--vm") == 0)
        {
            options.vm = 1;
        }
        else
        {
            goto bad_para;
        }
        btblk_name = argv[2];
        kernel_name = argv[3];
    }
    else if (argc == 5)
    {
        // ./createimage --extended --vm bootblock main
        if (strcmp(argv[1], "--extended") == 0)
        {
            options.extended = 1;
        }
        else if (strcmp(argv[1], "--vm") == 0)
        {
            options.vm = 1;
        }
        else
        {
            goto bad_para;
        }
        if (strcmp(argv[2], "--extended") == 0)
        {
            options.extended = 1;
        }
        else if (strcmp(argv[2], "--vm") == 0)
        {
            options.vm = 1;
        }
        else
        {
            goto bad_para;
        }
        btblk_name = argv[3];
        kernel_name = argv[4];
    }

    // the first one must be bootblock
    char *files[2];
    files[0] = btblk_name;
    files[1] = kernel_name;
    create_image(2, files);
    return 0;

bad_para:
{
    char par[256];
    int len = 0;
    for (int i = 1; i < argc; ++i)
    {
        for (int j = 0; argv[i][j]; ++j)
        {
            par[len++] = argv[i][j];
        }
        par[len++] = ' ';
    }
    par[len++] = '\0';
    error("Bad parameters: %s\n", par);
}
}

static void create_image(int nfiles, char *files[])
{
    char *btblk_name = files[0];
    char *kernel_name = files[1];

    FILE *btblk_file = fopen(btblk_name, "r");
    FILE *kernel_file = fopen(kernel_name, "r");
    FILE *image_file = fopen(IMAGE_FILE, "w+");

    if (btblk_file == NULL || kernel_file == NULL)
    {
        goto bad_file;
    }
    Elf32_Phdr *kn_phdr = read_exec_file(kernel_file);
    Elf32_Phdr *bb_phdr = read_exec_file(btblk_file);
    write_bootblock(image_file, btblk_file, bb_phdr);
    write_kernel(image_file, kernel_file, kn_phdr, kn_phdr->p_filesz);

    if (options.extended)
    {
        extent_opt(bb_phdr, kn_phdr);
    }
    free(kn_phdr);
    free(bb_phdr);
    fclose(image_file);
    fclose(btblk_file);
    fclose(kernel_file);

    image_file = fopen(IMAGE_FILE, "r+");
    record_kernel_sectors(image_file, kn_phdr->p_memsz);
    fclose(image_file);

    return;
bad_file:
{
    error("Wrong file names: %s %s, check and rerun!\n", btblk_name, kernel_name);
}
}

static Elf32_Phdr *read_exec_file(FILE *opfile)
{
    Elf32_Ehdr *ehdr;
    int _read_sz = 0;
    char buff[52];
    fread(buff, sizeof(Elf32_Ehdr), 1, opfile);
    _read_sz += sizeof(Elf32_Ehdr) * 1;
    ehdr = (Elf32_Ehdr *)buff;
    int _ph_sz_tot = ehdr->e_phentsize * ehdr->e_phnum;
    int _ph_sz = ehdr->e_phentsize;
    int _ph_num = ehdr->e_phnum;
    char *buff2 = (char *)malloc(_ph_sz_tot * sizeof(char));
    fread(buff2, sizeof(char), _ph_sz_tot, opfile);
    Elf32_Phdr *phdr = (Elf32_Phdr *)malloc(_ph_sz * sizeof(char));
    for (int i = 0; i < _ph_num; ++i)
    {
        Elf32_Phdr *tmp = (Elf32_Phdr *)buff2;
        if ((tmp + i * _ph_sz)->p_type == PT_LOAD)
        {
            *phdr = *(tmp + i * _ph_sz);
            break;
        }
    }
    free(buff2);
    _read_sz += sizeof(char) * _ph_sz_tot;
    int _off = phdr->p_offset;
    _read_sz = _off - _read_sz;
    if (_read_sz)
    {
        fread(buff, sizeof(char), _read_sz, opfile);
    }
    return phdr;
}

static uint32_t count_kernel_sectors(Elf32_Phdr *Phdr)
{
    Elf32_Word _fsz = Phdr->p_memsz;
    uint32_t cnt = (_fsz >> 9) + (_fsz & 0b111111111 ? 1 : 0);
    return cnt;
}

static void write_bootblock(FILE *image, FILE *file, Elf32_Phdr *phdr)
{
    fseek(file, phdr->p_offset, SEEK_SET);
    int _sz = phdr->p_memsz;
    char buff[512 + 5];
    memset(buff, 0, sizeof(buff));
    fseek(file, phdr->p_offset, SEEK_SET);
    fread(buff, sizeof(char), _sz, file);

    fseek(image, 0, SEEK_SET);
    fwrite(buff, sizeof(char), 512, image);
}

static void write_kernel(FILE *image, FILE *knfile, Elf32_Phdr *Phdr, int kernelsz)
{
    int sec_cnt = count_kernel_sectors(Phdr);
    char *buff = (char *)malloc(sec_cnt * 512 * sizeof(char));
    memset(buff, 0, sizeof(sec_cnt * 512 * sizeof(char)));
    fseek(knfile, Phdr->p_offset, SEEK_SET);
    fread(buff, sizeof(char), kernelsz, knfile);

    // you are supposed to clear the .bss section here...?

    fseek(image, 512, SEEK_SET);
    fwrite(buff, sizeof(char), sec_cnt * 512, image);
    free(buff);
}

static void record_kernel_sectors(FILE *image, int kernelsz)
{
    // when this function is called, the file ptr 'image' should points at
    // the exact beginning of the image file.
    int _sec_cnt = (kernelsz >> 9) + (kernelsz & 0b111111111 ? 1 : 0);
    int _os_sz = _sec_cnt * 512;
    char buf[5];
    buf[4] = 0;
    int byte_cnt = 0;
    fseek(image, 0, SEEK_SET);
    while (fread(buf, sizeof(char), 4, image) == 4)
    {
        byte_cnt += 4;
        fseek(image, -4, SEEK_CUR);
        if (byte_cnt == 512)
        {
            fwrite((char *)&_os_sz, sizeof(char), 4, image);
        }
        else
        {
            fwrite(buf, sizeof(char), 4, image);
        }
    }
}

static void extent_opt(Elf32_Phdr *Phdr_bb, Elf32_Phdr *Phdr_k)
{
    printf("bootblock info:\n");
    printf("\toffset: 0x%x\n", Phdr_bb->p_offset);
    printf("\tfile size: 0x%x\n", Phdr_bb->p_filesz);
    printf("\tmemory size: 0x%x\n", Phdr_bb->p_memsz);
    printf("\tvaddr: 0x%x\n", Phdr_bb->p_vaddr);
    printf("\tpadding upto 0x%x\n", Phdr_bb->p_vaddr + 512);

    printf("kernel info:\n");
    printf("\toffset: 0x%x\n", Phdr_k->p_offset);
    printf("\tfile size: 0x%x\n", Phdr_k->p_filesz);
    printf("\tmemory size: 0x%x\n", Phdr_k->p_memsz);
    printf("\tvaddr: 0x%x\n", Phdr_k->p_vaddr);
    printf("\tpadding upto 0x%x\n", Phdr_k->p_vaddr + 512 * count_kernel_sectors(Phdr_k));
}

// print the usage of this executable
static void print_usage()
{
    printf("Usage: ./createimage" ARGS "\n");
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    exit(-1);
}
