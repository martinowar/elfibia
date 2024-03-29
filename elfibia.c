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

typedef struct
{
    int elf_file_desc;
    size_t menu_item_count;
    Elf *sElf;
    item_data *main_menu_data;
} efb_context;

efb_context efb_ctx;

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

    if ((efb_ctx->elf_file_desc = open(argv[1], O_RDONLY, 0)) < 0)
    {
        printf("Cannot open %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if ((efb_ctx->sElf = elf_begin(efb_ctx->elf_file_desc, ELF_C_READ, NULL)) == NULL)
    {
        printf("elf_begin() failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    if (elf_kind(efb_ctx->sElf) != ELF_K_ELF)
    {
        printf("%s is not an ELF object\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    efb_ctx->main_menu_data = NULL;
}

char * efb_get_menu_item_content(const int menu_item_idx)
{
    content_buf[0] = '\0';

    if (menu_item_idx == MENU_IDX_ELF_HEADER)
    {
        efb_get_elf_header(efb_ctx.sElf, content_buf);
    }
    else if (menu_item_idx == MENU_IDX_SEGMENTS_SUMMARY)
    {
        efb_get_segment_content(efb_ctx.sElf, content_buf);
    }
    else if (menu_item_idx == MENU_IDX_SECTIONS_SUMMARY)
    {
        // TODO
        return "FIX ME: return a sections' summary";
    }
    else
    {
        efb_get_section_content(efb_ctx.sElf, menu_item_idx - MENU_IDX_FIRST_SECTION, content_buf);
    }

    return content_buf;
}

static void efb_close(efb_context *efb_ctx)
{
    if (efb_ctx->sElf != NULL)
    {
        elf_end(efb_ctx->sElf);
    }

    close(efb_ctx->elf_file_desc);

    if (efb_ctx->main_menu_data != NULL)
    {
        free(efb_ctx->main_menu_data);
    }
}

int main(int argc, char **argv)
{
    efb_init(&efb_ctx, argc, argv);

    efb_ctx.menu_item_count = MENU_IDX_FIRST_SECTION + efb_get_sect_count(efb_ctx.sElf);
    efb_ctx.main_menu_data = malloc(efb_ctx.menu_item_count * sizeof(item_data));
    efb_ctx.main_menu_data[MENU_IDX_ELF_HEADER] = (item_data) {"ELF Header", "<info>"};
    efb_ctx.main_menu_data[MENU_IDX_SEGMENTS_SUMMARY] = (item_data) {"Segments", "<info>"};
    efb_ctx.main_menu_data[MENU_IDX_SECTIONS_SUMMARY] = (item_data) {"Sections", "<info"};
    efb_get_sect_name_and_type(efb_ctx.sElf, &efb_ctx.main_menu_data[MENU_IDX_FIRST_SECTION]);

    efb_draw_view(efb_ctx.main_menu_data, efb_ctx.menu_item_count);

    efb_close(&efb_ctx);
    return 0;
}
