#ifndef ELFIBIA_H_INCLUDED
#define ELFIBIA_H_INCLUDED

#include <stdio.h>
#include <gelf.h>

void efb_get_sect_names(Elf *sElf, char * sect_names[]);
size_t efb_get_sect_count(Elf *sElf);

void efb_draw_view(char **menu_strings, const int menu_items_count);

char * efb_get_menu_item_content(const int menu_item_idx);

void efb_get_section_content(Elf *sElf, const int section_idx, char * out_buffer);

void efb_get_elf_header(Elf * sElf, char * out_buffer);

void efb_get_segment_content(Elf *sElf, char * out_buffer);

#endif // ELFIBIA_H_INCLUDED
