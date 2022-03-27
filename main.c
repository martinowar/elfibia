#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <gelf.h>

typedef struct
{
    int file_desc;
    Elf *sElf;
    // TODO  check if works with 32-bit ELF files
    Elf64_Ehdr *elf_header;
    char **sect_names;
} efb_context;

void efb_init(efb_context *efb_ctx, int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s file-name\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (elf_version(EV_CURRENT) == EV_NONE)
    {
        printf("ELF library initialization failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    if ((efb_ctx->file_desc = open(argv[1], O_RDONLY, 0)) < 0)
    {
        printf("Cannot open %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if ((efb_ctx->sElf = elf_begin(efb_ctx->file_desc, ELF_C_READ, NULL)) == NULL)
    {
        printf("elf_begin() failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    if (elf_kind(efb_ctx->sElf) != ELF_K_ELF)
    {
        printf("%s is not an ELF object\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    efb_ctx->sect_names = NULL;
}

void efb_close(efb_context *efb_ctx)
{
    elf_end(efb_ctx->sElf);
    close(efb_ctx->file_desc);

    free(efb_ctx->sect_names);
}

char * efb_get_sect_name(efb_context *efb_ctx, Elf64_Word sect_name_offset)
{
    char *sect_name = NULL;

    if ((sect_name = elf_strptr(efb_ctx->sElf, efb_ctx->elf_header->e_shstrndx, sect_name_offset)) == NULL)
    {
        printf("elf_strptr() failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    return sect_name;
}

void efb_get_elf_header(efb_context *efb_ctx)
{
    efb_ctx->elf_header = elf64_getehdr(efb_ctx->sElf);
}

void efb_get_sect_names(efb_context *efb_ctx)
{
    if (efb_ctx->sect_names == NULL)
    {
        // TODO move this initialization in efb_init?
        efb_get_elf_header(efb_ctx);

        efb_ctx->sect_names = malloc(efb_ctx->elf_header->e_shnum * sizeof(char *));

        Elf_Scn *sect = NULL;
        while ((sect = elf_nextscn(efb_ctx->sElf, sect)) != NULL)
        {
            GElf_Shdr sect_header;
            if (gelf_getshdr(sect, &sect_header) != &sect_header)
            {
                printf("getshdr() failed: %s\n", elf_errmsg(-1));
                exit(EXIT_FAILURE);
            }

            efb_ctx->sect_names[elf_ndxscn(sect)] = efb_get_sect_name(efb_ctx, sect_header.sh_name);
        }
    }
    else
    {
        printf("efb_get_sect_names: already initialized\n");
    }
}

int main(int argc, char **argv)
{
    efb_context efb_ctx;
    efb_init(&efb_ctx, argc, argv);
    efb_get_sect_names(&efb_ctx);


    // TODO just for test
    for (intmax_t idx = 0; idx < efb_ctx.elf_header->e_shnum; idx++)
    {
        printf("Section %-4.4jd %s\n", idx, efb_ctx.sect_names[idx]);
    }


    efb_close(&efb_ctx);

    return 0;
}
