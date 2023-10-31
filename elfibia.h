#ifndef ELFIBIA_H_INCLUDED
#define ELFIBIA_H_INCLUDED

#include <stdio.h>
#include <gelf.h>

typedef struct
{
    char * item_name;
    char * item_descr;
} item_data;

void efb_get_sect_name_and_type(Elf *sElf, item_data * it_data);
size_t efb_get_sect_count(Elf *sElf);

void efb_draw_view(item_data *it_data, const int menu_items_count);

char * efb_get_menu_item_content(const int menu_item_idx);

void efb_get_section_content(Elf *sElf, const int section_idx, char * out_buffer);

void efb_get_elf_header(Elf * sElf, char * out_buffer);

void efb_get_segment_content(Elf *sElf, char * out_buffer);

#endif // ELFIBIA_H_INCLUDED
