#ifndef ELFIBIA_H_INCLUDED
#define ELFIBIA_H_INCLUDED

#include <stdio.h>
#include <gelf.h>

typedef struct
{
    int file_desc;
    Elf *sElf;
    size_t sect_hdr_strtbl_idx;
    size_t sect_count;
    char **sect_names;
} efb_context;

void efb_get_sect_names(efb_context *efb_ctx);
void efb_get_sect_count(efb_context *efb_ctx);

void efb_draw_view(char **menu_strings, const int menu_items_count);

char * get_menu_item_content(const int menu_item_idx);

char * get_section_content(efb_context *efb_ctx, const int section_idx);

#endif // ELFIBIA_H_INCLUDED
