#include "elfibia.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <gelf.h>

efb_context efb_ctx;

static void efb_get_secthdr_strtbl_idx(efb_context *efb_ctx)
{
    if (elf_getshdrstrndx(efb_ctx->sElf, &efb_ctx->sect_hdr_strtbl_idx) != 0)
    {
        printf("elf_getshdrstrndx() failed: %s", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }
}

static void efb_init(efb_context *efb_ctx, int argc, char **argv)
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
    efb_get_secthdr_strtbl_idx(efb_ctx);
    efb_get_sect_count(efb_ctx);
}

char * get_menu_item_content(const int menu_item_idx)
{
    return efb_ctx.sect_names[menu_item_idx];
}

static void efb_close(efb_context *efb_ctx)
{
    elf_end(efb_ctx->sElf);
    close(efb_ctx->file_desc);

    free(efb_ctx->sect_names);
}

int main(int argc, char **argv)
{
    efb_init(&efb_ctx, argc, argv);
    efb_get_sect_names(&efb_ctx);


    // TODO just for test
//    for (intmax_t idx = 0; idx < efb_ctx.sect_count; idx++)
//    {
//        printf("Section %-4.4jd %s\n", idx, efb_ctx.sect_names[idx]);
//    }

    efb_draw_view(efb_ctx.sect_names, efb_ctx.sect_count);

    efb_close(&efb_ctx);

    return 0;
}
