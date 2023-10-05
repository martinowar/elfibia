#include "elfibia.h"

#include <elf.h>
#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DUMP_ROW_WIDTH 16
#define DUMP_COL_WIDTH 4
#define BUF_HEX_SIZE   (2 * DUMP_ROW_WIDTH + DUMP_COL_WIDTH + 1)

#define content_buf_size 1000000

static char content_buf[content_buf_size];

void print_secthdr_struct(GElf_Shdr *elfShdr, char * out_buffer)
{
    sprintf(&out_buffer[strlen(out_buffer)],
        "sh_addr      = %lx\n"
        "sh_addralign = %ld\n"
        "sh_entsize   = %ld\n"
        "sh_flags     = %lx\n"
        "sh_info      = %d\n"
        "sh_link      = %d\n"
        "sh_name      = %d\n"
        "sh_offset    = %ld\n"
        "sh_size      = %ld\n"
        "sh_type      = %d\n\n",
        elfShdr->sh_addr, elfShdr->sh_addralign, elfShdr->sh_entsize,
        elfShdr->sh_flags, elfShdr->sh_info, elfShdr->sh_link,
        elfShdr->sh_name, elfShdr->sh_offset, elfShdr->sh_size, elfShdr->sh_type);
}

void print_elf_data_struct(Elf_Data *elf_data, char * out_buffer)
{
    sprintf(&out_buffer[strlen(out_buffer)],
        "d_buf     = %p\n"
        "d_type    = %d\n"
        "d_version = %d\n"
        "d_size    = %ld\n"
        "d_off     = %ld\n"
        "d_align   = %ld\n\n",
        elf_data->d_buf, elf_data->d_type, elf_data->d_version,
        elf_data->d_size, elf_data->d_off, elf_data->d_align);
}

void dump_sect_data(Elf_Data *elf_data, GElf_Addr sect_addr, char * out_buffer)
{
    size_t data_index = 0;
    size_t row_index = 0;
    size_t buf_hex_index = 0;
    size_t buf_char_index = 0;
    const char *ptr_data = elf_data->d_buf;
    char buf_hex[BUF_HEX_SIZE];
    char buf_char[DUMP_ROW_WIDTH + 1];

    while (ptr_data < ((char *)elf_data->d_buf + elf_data->d_size))
    {
        sprintf(&buf_hex[buf_hex_index], "%02x", *ptr_data);
        buf_hex_index += 2;
        sprintf(&buf_char[buf_char_index++], "%c", (*ptr_data < ' ' || *ptr_data > '~') ? '.' : *ptr_data);
        data_index++;
        row_index++;
        ptr_data++;
        if ((data_index % DUMP_COL_WIDTH) == 0)
        {
            sprintf(&buf_hex[buf_hex_index++], "%c", ' ');
        }

        if (((row_index % DUMP_ROW_WIDTH) == 0) || (data_index >= elf_data->d_size))
        {
            buf_hex_index = 0;
            buf_char_index = 0;
            int bufPadding = BUF_HEX_SIZE - buf_hex_index;

            // TODO it should be 32 and 64 bit compatible (depending on the sect_addr type)
            sprintf(&out_buffer[strlen(out_buffer)], "  0x%08lx %*s%s\n", sect_addr + data_index - row_index, -bufPadding, buf_hex, buf_char);
            row_index = 0;
        }
    }

//    putchar('\n');
}

void dump_sect_strings(Elf_Data *elf_data, GElf_Addr sect_addr, char * out_buffer)
{
    const char *ptr_data = elf_data->d_buf;
    size_t data_index = 0;

    if ((*ptr_data == 0) && (elf_data->d_size > 0))
    {
        sprintf(&out_buffer[strlen(out_buffer)], "  [%6d]  %s\n", 0, "(empty string)");
        ptr_data++;
        data_index++;
    }

    while (ptr_data < ((char *)elf_data->d_buf + elf_data->d_size))
    {
        size_t str_size = strlen(ptr_data) + 1;
        sprintf(&out_buffer[strlen(out_buffer)], "  [%6ld]  %s\n", data_index, ptr_data);
        ptr_data += str_size;
        data_index += str_size;
    }

//    putchar('\n');
}

static void info_sect_strtab(Elf_Scn * elf_sect, GElf_Shdr *sect_header, const bool dump_data, char * sect_name, char * out_buffer)
{
    if (sect_header->sh_type == SHT_STRTAB)
    {
        print_secthdr_struct(sect_header, out_buffer);

        Elf_Data *elf_data = NULL;
        if ((elf_data = elf_getdata(elf_sect, elf_data)) == NULL)
        {
            errx(EXIT_FAILURE, "elf_getdata() failed: %s.", elf_errmsg(-1));
        }

        print_elf_data_struct(elf_data, out_buffer);
        dump_sect_strings(elf_data, sect_header->sh_addr, out_buffer);

        if (dump_data == true)
        {
            dump_sect_data(elf_data, sect_header->sh_addr, out_buffer);
        }
    }
}

static char * efb_get_sect_name(efb_context *efb_ctx, Elf64_Word sect_name_offset)
{
    char *sect_name = NULL;

    if ((sect_name = elf_strptr(efb_ctx->sElf, efb_ctx->sect_hdr_strtbl_idx, sect_name_offset)) == NULL)
    {
        printf("elf_strptr() failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    return sect_name;
}

void efb_get_sect_names(efb_context *efb_ctx)
{
    if (efb_ctx->sect_names == NULL)
    {
        efb_ctx->sect_names = malloc(efb_ctx->sect_count * sizeof(char *));

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

void efb_get_sect_count(efb_context *efb_ctx)
{
   if (elf_getshdrnum(efb_ctx->sElf, &efb_ctx->sect_count) != 0)
   {
        printf("elf_getshdrnum() failed: %s", elf_errmsg(-1));
        exit(EXIT_FAILURE);
   }
}

char * get_section_content(efb_context *efb_ctx, const int section_idx)
{
    char * section_name = NULL;
    Elf_Scn *sect = NULL;

    memset(content_buf, 0, content_buf_size);

    while ((sect = elf_nextscn(efb_ctx->sElf, sect)) != NULL)
    {
        GElf_Shdr sect_header;
        if (gelf_getshdr(sect, &sect_header) != &sect_header)
        {
            errx(EXIT_FAILURE, "getshdr() failed: %s.", elf_errmsg(-1));
        }

        if (elf_ndxscn(sect) == section_idx)
        {
            section_name = efb_ctx->sect_names[section_idx];
//        (void) printf("Section %-4.4jd %s\n", (uintmax_t)elf_ndxscn(sect), section_name);
//        info_sect_dynamic(efb_ctx->sElf, sect, &sect_header, false, section_name);
            info_sect_strtab(sect, &sect_header, true, section_name, content_buf);
        }
    }

    return content_buf[0] ? content_buf : "(empty)";
}
