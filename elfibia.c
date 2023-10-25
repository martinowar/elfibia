#include "elfibia.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <gelf.h>

#define MENU_IDX_ELF_HEADER 0
#define MENU_IDX_SEGMENTS_SUMMARY 1
#define MENU_IDX_SECTIONS_SUMMARY 2
#define MENU_IDX_FIRST_SECTION (MENU_IDX_SECTIONS_SUMMARY + 1)

// TODO Use a dynamic buffer
#define CONTENT_BUF_SIZE 5000000
static char content_buf[CONTENT_BUF_SIZE];

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

    efb_ctx->menu_items = NULL;
    efb_get_secthdr_strtbl_idx(efb_ctx);
}

char * efb_get_menu_item_content(const int menu_item_idx)
{
    content_buf[0] = '\0';

    if (menu_item_idx == MENU_IDX_ELF_HEADER)
    {
        efb_get_elf_header(&efb_ctx, content_buf);
    }
    else if (menu_item_idx == MENU_IDX_SEGMENTS_SUMMARY)
    {
        efb_get_segment_content(&efb_ctx, content_buf);
    }
    else if (menu_item_idx == MENU_IDX_SECTIONS_SUMMARY)
    {
        // TODO
        return "FIX ME: return a sections' summary";
    }
    else
    {
        efb_get_section_content(&efb_ctx, menu_item_idx - MENU_IDX_FIRST_SECTION, content_buf);
    }

    return content_buf;
}

static void efb_close(efb_context *efb_ctx)
{
    if (efb_ctx->sElf != NULL)
    {
        elf_end(efb_ctx->sElf);
    }

    close(efb_ctx->file_desc);

    if (efb_ctx->menu_items != NULL)
    {
        free(efb_ctx->menu_items);
    }
}

int main(int argc, char **argv)
{
    efb_init(&efb_ctx, argc, argv);

    size_t menu_items_count = MENU_IDX_FIRST_SECTION + efb_get_sect_count(&efb_ctx);
    efb_ctx.menu_items = malloc(menu_items_count * sizeof(char *));
    efb_ctx.menu_items[MENU_IDX_ELF_HEADER] = "ELF Header";
    efb_ctx.menu_items[MENU_IDX_SEGMENTS_SUMMARY] = "Segments";
    efb_ctx.menu_items[MENU_IDX_SECTIONS_SUMMARY] = "Sections";
    efb_get_sect_names(&efb_ctx, &efb_ctx.menu_items[MENU_IDX_FIRST_SECTION]);

    efb_draw_view(efb_ctx.menu_items, menu_items_count);

    efb_close(&efb_ctx);
    return 0;
}
