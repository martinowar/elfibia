#include "elfibia.h"

#include <elf.h>
#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DUMP_ROW_WIDTH 16
#define DUMP_COL_WIDTH 4
#define BUF_HEX_SIZE   (2 * DUMP_ROW_WIDTH + DUMP_COL_WIDTH + 1)

static void get_secthdr_struct(GElf_Shdr *elfShdr, char * out_buffer)
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

static void get_elf_data_struct(Elf_Data *elf_data, char * out_buffer)
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

static void dump_sect_data(Elf_Data *elf_data, GElf_Addr sect_addr, char * out_buffer)
{
    size_t data_index = 0;
    size_t row_index = 0;
    size_t buf_hex_index = 0;
    size_t buf_char_index = 0;
    const char *ptr_data = elf_data->d_buf;
    char buf_hex[BUF_HEX_SIZE];
    char buf_char[DUMP_ROW_WIDTH + 1];

    if (elf_data->d_buf != NULL)
    {
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
                int buf_padding = BUF_HEX_SIZE - buf_hex_index;

                // TODO it should be 32 and 64 bit compatible (depending on the sect_addr type)
                sprintf(&out_buffer[strlen(out_buffer)], "  0x%08lx %*s%s\n", sect_addr + data_index - row_index, -buf_padding, buf_hex, buf_char);
                row_index = 0;
            }
        }
    }
    else
    {
        sprintf(&out_buffer[strlen(out_buffer)], "The section has no data to dump.\n");
    }
}

static void dump_sect_strings(Elf_Data *elf_data, GElf_Addr sect_addr, char * out_buffer)
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
}

char * get_dynamic_type(const long int dynType)
{
#define CS_DT(TYPE) case DT_##TYPE: return #TYPE

    switch (dynType)
    {
        CS_DT(NULL);            CS_DT(NEEDED);          CS_DT(PLTRELSZ);
        CS_DT(PLTGOT);          CS_DT(HASH);            CS_DT(STRTAB);
        CS_DT(SYMTAB);          CS_DT(RELA);            CS_DT(RELASZ);
        CS_DT(RELAENT);         CS_DT(STRSZ);           CS_DT(SYMENT);
        CS_DT(INIT);            CS_DT(FINI);            CS_DT(SONAME);
        CS_DT(RPATH);           CS_DT(SYMBOLIC);        CS_DT(REL);
        CS_DT(RELSZ);           CS_DT(RELENT);          CS_DT(PLTREL);
        CS_DT(DEBUG);           CS_DT(TEXTREL);         CS_DT(JMPREL);
        CS_DT(BIND_NOW);        CS_DT(INIT_ARRAY);      CS_DT(FINI_ARRAY);
        CS_DT(INIT_ARRAYSZ);    CS_DT(FINI_ARRAYSZ);    CS_DT(RUNPATH);
        CS_DT(FLAGS);           CS_DT(PREINIT_ARRAY);   CS_DT(PREINIT_ARRAYSZ);
        CS_DT(SYMTAB_SHNDX);    CS_DT(NUM);             CS_DT(GNU_HASH);
        CS_DT(VERNEEDNUM);      CS_DT(VERNEED);         CS_DT(VERSYM);
        default:
            return "unknown";
    }

#undef CS_DT
}

char * get_dyn_symbol_val(Elf *sElf, GElf_Shdr *sect_header, GElf_Dyn * elf_dyn_symbol, char * sym_val)
{
#define CS_DT_VAL_HEX(TAG) case DT_##TAG: sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val); break
#define CS_DT_VAL_BYTES(TAG) case DT_##TAG: sprintf(sym_val, "%ld (bytes)", elf_dyn_symbol->d_un.d_val); break
#define CS_DT_VAL_DEC(TAG) case DT_##TAG: sprintf(sym_val, "%ld", elf_dyn_symbol->d_un.d_val); break

    switch (elf_dyn_symbol->d_tag)
    {
        CS_DT_VAL_HEX(INIT);            CS_DT_VAL_HEX(FINI);            CS_DT_VAL_HEX(INIT_ARRAY);
        CS_DT_VAL_BYTES(INIT_ARRAYSZ);  CS_DT_VAL_HEX(FINI_ARRAY);      CS_DT_VAL_BYTES(FINI_ARRAYSZ);
        CS_DT_VAL_HEX(GNU_HASH);        CS_DT_VAL_HEX(STRTAB);          CS_DT_VAL_HEX(SYMTAB);
        CS_DT_VAL_BYTES(STRSZ);         CS_DT_VAL_BYTES(SYMENT);        CS_DT_VAL_HEX(DEBUG);
        CS_DT_VAL_HEX(PLTGOT);          CS_DT_VAL_BYTES(PLTRELSZ);      CS_DT_VAL_HEX(JMPREL);
        CS_DT_VAL_HEX(RELA);            CS_DT_VAL_BYTES(RELASZ);        CS_DT_VAL_BYTES(RELAENT);
        CS_DT_VAL_HEX(VERNEED);         CS_DT_VAL_DEC(VERNEEDNUM);      CS_DT_VAL_HEX(VERSYM);

        case DT_NEEDED:
            char *sym_name;
            if ((sym_name = elf_strptr(sElf, sect_header->sh_link, elf_dyn_symbol->d_un.d_val)) == NULL)
            {
                errx(EXIT_FAILURE, "elf_strptr() failed: %s.", elf_errmsg(-1));
            }

            sprintf(sym_val, "Shared library: [%s]", sym_name);
            break;

    }

    return sym_val;

#undef CS_DT_VAL_HEX
#undef CS_DT_VAL_BYTES
#undef CS_DT_VAL_DEC
}

void info_sect_dynamic(Elf *sElf, Elf_Scn * sect, GElf_Shdr *sect_header, const bool dump_data, char * sect_name, char * out_buffer)
{
    if (sect_header->sh_type == SHT_DYNAMIC)
    {
        get_secthdr_struct(sect_header, out_buffer);

        Elf_Data *elf_data = NULL;
        if ((elf_data = elf_getdata(sect, elf_data)) == NULL)
        {
            errx(EXIT_FAILURE, "elf_getdata() failed: %s.", elf_errmsg(-1));
        }

        printf("%-19s %-18s %s\n", " Tag", "Type", "Name / Value");
        GElf_Dyn elf_dyn_symbol;
        for (int idx = 0; idx < (sect_header->sh_size / sect_header->sh_entsize); idx++)
        {
            if (gelf_getdyn(elf_data, idx, &elf_dyn_symbol) == NULL)
            {
                errx(EXIT_FAILURE, "gelf_getsym() failed: %s.", elf_errmsg(-1));
            }

            if (elf_dyn_symbol.d_tag != DT_NULL)
            {
                char sym_val[100] = {"***fixme***"};
                sprintf(&out_buffer[strlen(out_buffer)], " 0x%016lx %-18s %s\n",
                        elf_dyn_symbol.d_tag,
                        get_dynamic_type(elf_dyn_symbol.d_tag),
                        get_dyn_symbol_val(sElf, sect_header, &elf_dyn_symbol, sym_val));
            }
        }

        putchar('\n');

        get_elf_data_struct(elf_data, out_buffer);

        if (dump_data == true)
        {
            dump_sect_data(elf_data, sect_header->sh_addr, out_buffer);
        }
    }
}

static void info_sect_strtab(Elf_Scn * elf_sect, GElf_Shdr *sect_header, const bool dump_data, char * sect_name, char * out_buffer)
{
    if (sect_header->sh_type == SHT_STRTAB)
    {
        get_secthdr_struct(sect_header, out_buffer);

        Elf_Data *elf_data = NULL;
        if ((elf_data = elf_getdata(elf_sect, elf_data)) == NULL)
        {
            errx(EXIT_FAILURE, "elf_getdata() failed: %s.", elf_errmsg(-1));
        }

        get_elf_data_struct(elf_data, out_buffer);
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

void efb_get_sect_names(efb_context *efb_ctx, char * sect_names[])
{
    Elf_Scn *sect = NULL;
    while ((sect = elf_nextscn(efb_ctx->sElf, sect)) != NULL)
    {
        GElf_Shdr sect_header;
        if (gelf_getshdr(sect, &sect_header) != &sect_header)
        {
            printf("getshdr() failed: %s\n", elf_errmsg(-1));
            exit(EXIT_FAILURE);
        }

        sect_names[elf_ndxscn(sect)] = efb_get_sect_name(efb_ctx, sect_header.sh_name);
    }
}

size_t efb_get_sect_count(efb_context *efb_ctx)
{
    size_t section_count;
    if (elf_getshdrnum(efb_ctx->sElf, &section_count) != 0)
    {
        printf("elf_getshdrnum() failed: %s", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    return section_count;
}

void efb_get_section_content(efb_context *efb_ctx, const int section_idx, char * out_buffer)
{
    char * section_name = NULL;
    Elf_Scn *sect = NULL;

    while ((sect = elf_nextscn(efb_ctx->sElf, sect)) != NULL)
    {
        GElf_Shdr sect_header;
        if (gelf_getshdr(sect, &sect_header) != &sect_header)
        {
            errx(EXIT_FAILURE, "getshdr() failed: %s.", elf_errmsg(-1));
        }

        if (elf_ndxscn(sect) == section_idx)
        {
            section_name = "FIX_ME"; //efb_ctx->sect_names[section_idx];
            sprintf(out_buffer, "Section %-4.4jd %s\n", (uintmax_t)elf_ndxscn(sect), section_name);
            switch (sect_header.sh_type)
            {
            case SHT_DYNAMIC:
                info_sect_dynamic(efb_ctx->sElf, sect, &sect_header, true, section_name, out_buffer);
                break;
            case SHT_STRTAB:
                info_sect_strtab(sect, &sect_header, true, section_name, out_buffer);
                break;
            default:
                get_secthdr_struct(&sect_header, out_buffer);

                Elf_Data *elf_data = NULL;
                if ((elf_data = elf_getdata(sect, elf_data)) == NULL)
                {
                    errx(EXIT_FAILURE, "elf_getdata() failed: %s.", elf_errmsg(-1));
                }

                get_elf_data_struct(elf_data, out_buffer);

                dump_sect_data(elf_data, sect_header.sh_addr, out_buffer);
                break;
            }
        }
    }
}
