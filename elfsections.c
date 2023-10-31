#include "elfibia.h"

#include <elf.h>
#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DUMP_ROW_WIDTH 16
#define DUMP_COL_WIDTH 4
#define BUF_HEX_SIZE   (2 * DUMP_ROW_WIDTH + DUMP_COL_WIDTH + 1)

static size_t get_secthdr_strtbl_idx(Elf *sElf)
{
    size_t sect_hdr_strtbl_idx;

    if (elf_getshdrstrndx(sElf, &sect_hdr_strtbl_idx) != 0)
    {
        printf("elf_getshdrstrndx() failed: %s", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    return sect_hdr_strtbl_idx;
}

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
        "sh_type      = 0x%x\n\n",
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

static char * efb_get_section_type(const long int sect_type)
{
    switch (sect_type)
    {
        case SHT_NULL:
            return "NULL";
        case SHT_PROGBITS:
            return "PROGBITS";
        case SHT_SYMTAB:
            return "SYMTAB";
        case SHT_STRTAB:
            return "STRTAB";
        case SHT_RELA:
            return "RELA";
        case SHT_HASH:
            return "HASH";
        case SHT_DYNAMIC:
            return "DYNAMIC";
        case SHT_NOTE:
            return "NOTE";
        case SHT_NOBITS:
            return "NOBITS";
        case SHT_REL:
            return "REL";
        case SHT_SHLIB:
            return "SHLIB";
        case SHT_DYNSYM:
            return "DYNSYM";
        case SHT_INIT_ARRAY:
            return "INIT_ARRAY";
        case SHT_FINI_ARRAY:
            return "FINI_ARRAY";
        case SHT_PREINIT_ARRAY:
            return "PREINIT_ARRAY";
        case SHT_GROUP:
            return "GROUP";
        case SHT_SYMTAB_SHNDX:
            return "SYMTAB_SHNDX";
        case SHT_RELR:
            return "RELR";
        default:
            return "<unknown>";
    }
}

char * get_dynamic_type(const long int dyn_type)
{
    switch (dyn_type)
    {
        case DT_NULL:
            return "NULL";
        case DT_NEEDED:
            return "NEEDED";
        case DT_PLTRELSZ:
            return "PLTRELSZ";
        case DT_PLTGOT:
            return "PLTGOT";
        case DT_HASH:
            return "HASH";
        case DT_STRTAB:
            return "STRTAB";
        case DT_SYMTAB:
            return "SYMTAB";
        case DT_RELA:
            return "RELA";
        case DT_RELASZ:
            return "RELASZ";
        case DT_RELAENT:
            return "RELAENT";
        case DT_STRSZ:
            return "STRSZ";
        case DT_SYMENT:
            return "SYMENT";
        case DT_INIT:
            return "INIT";
        case DT_FINI:
            return "FINI";
        case DT_SONAME:
            return "SONAME";
        case DT_RPATH:
            return "RPATH";
        case DT_SYMBOLIC:
            return "SYMBOLIC";
        case DT_REL:
            return "REL";
        case DT_RELSZ:
            return "RELSZ";
        case DT_RELENT:
            return "RELENT";
        case DT_PLTREL:
            return "PLTREL";
        case DT_DEBUG:
            return "DEBUG";
        case DT_TEXTREL:
            return "TEXTREL";
        case DT_JMPREL:
            return "JMPREL";
        case DT_BIND_NOW:
            return "BIND_NOW";
        case DT_INIT_ARRAY:
            return "INIT_ARRAY";
        case DT_FINI_ARRAY:
            return "FINI_ARRAY";
        case DT_INIT_ARRAYSZ:
            return "INIT_ARRAYSZ";
        case DT_FINI_ARRAYSZ:
            return "FINI_ARRAYSZ";
        case DT_RUNPATH:
            return "RUNPATH";
        case DT_FLAGS:
            return "FLAGS";
        case DT_PREINIT_ARRAY:
            return "PREINIT_ARRAY";
        case DT_PREINIT_ARRAYSZ:
            return "PREINIT_ARRAYSZ";
        case DT_SYMTAB_SHNDX:
            return "SYMTAB_SHNDX";
        case DT_NUM:
            return "NUM";
        case DT_GNU_HASH:
            return "GNU_HASH";
        case DT_VERNEEDNUM:
            return "VERNEEDNUM";
        case DT_VERNEED:
            return "VERNEED";
        case DT_VERSYM:
            return "VERSYM";
        default:
            return "<unknown>";
    }
}

char * get_dyn_symbol_val(Elf *sElf, GElf_Shdr *sect_header, GElf_Dyn * elf_dyn_symbol, char * sym_val)
{
    switch (elf_dyn_symbol->d_tag)
    {
        case DT_INIT:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_FINI:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_INIT_ARRAY:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_INIT_ARRAYSZ:
            sprintf(sym_val, "%ld (bytes)", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_FINI_ARRAY:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_FINI_ARRAYSZ:
            sprintf(sym_val, "%ld (bytes)", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_GNU_HASH:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_PLTREL:
            sprintf(sym_val, "%s", get_dynamic_type(elf_dyn_symbol->d_un.d_val));
            break;
        case DT_STRTAB:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_SYMTAB:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_STRSZ:
            sprintf(sym_val, "%ld (bytes)", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_SYMENT:
            sprintf(sym_val, "%ld (bytes)", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_DEBUG:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_PLTGOT:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_PLTRELSZ:
            sprintf(sym_val, "%ld (bytes)", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_JMPREL:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_RELA:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_RELASZ:
            sprintf(sym_val, "%ld (bytes)", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_RELAENT:
            sprintf(sym_val, "%ld (bytes)", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_VERNEED:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_VERNEEDNUM:
            sprintf(sym_val, "%ld", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_VERSYM:
            sprintf(sym_val, "0x%lx", elf_dyn_symbol->d_un.d_val);
            break;
        case DT_NEEDED:
            char *sym_name;
            if ((sym_name = elf_strptr(sElf, sect_header->sh_link, elf_dyn_symbol->d_un.d_val)) == NULL)
            {
                errx(EXIT_FAILURE, "elf_strptr() failed: %s.", elf_errmsg(-1));
            }

            sprintf(sym_val, "Shared library: [%s]", sym_name);
            break;
        default:
            // Empty
            break;

    }

    return sym_val;
}

void info_sect_dynamic(Elf *sElf, Elf_Scn * sect, GElf_Shdr *sect_header, const bool dump_data, char * out_buffer)
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
                // TODO check if the size is enough
                // Add a guard condition
                char sym_val[100] = { "<unknown>" };
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

static void info_sect_strtab(Elf_Scn * elf_sect, GElf_Shdr *sect_header, const bool dump_data, char * out_buffer)
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

static char * efb_get_sect_name(Elf *sElf, Elf64_Word sect_name_offset)
{
    char *sect_name = NULL;

    if ((sect_name = elf_strptr(sElf, get_secthdr_strtbl_idx(sElf), sect_name_offset)) == NULL)
    {
        printf("elf_strptr() failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    return sect_name;
}

void efb_get_sect_name_and_type(Elf *sElf, item_data * it_data)
{
    Elf_Scn *sect = NULL;
    while ((sect = elf_nextscn(sElf, sect)) != NULL)
    {
        GElf_Shdr sect_header;
        if (gelf_getshdr(sect, &sect_header) != &sect_header)
        {
            printf("getshdr() failed: %s\n", elf_errmsg(-1));
            exit(EXIT_FAILURE);
        }

        it_data[elf_ndxscn(sect)].item_name = efb_get_sect_name(sElf, sect_header.sh_name);
        it_data[elf_ndxscn(sect)].item_descr = efb_get_section_type(sect_header.sh_type);
    }
}

size_t efb_get_sect_count(Elf *sElf)
{
    size_t section_count;
    if (elf_getshdrnum(sElf, &section_count) != 0)
    {
        printf("elf_getshdrnum() failed: %s", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    return section_count;
}

void efb_get_section_content(Elf *sElf, const int section_idx, char * out_buffer)
{
    Elf_Scn *sect = NULL;

    while ((sect = elf_nextscn(sElf, sect)) != NULL)
    {
        GElf_Shdr sect_header;
        if (gelf_getshdr(sect, &sect_header) != &sect_header)
        {
            errx(EXIT_FAILURE, "getshdr() failed: %s.", elf_errmsg(-1));
        }

        if (elf_ndxscn(sect) == section_idx)
        {
            sprintf(out_buffer, "Section %jd\n", (uintmax_t)elf_ndxscn(sect));
            switch (sect_header.sh_type)
            {
            case SHT_DYNAMIC:
                info_sect_dynamic(sElf, sect, &sect_header, true, out_buffer);
                break;
            case SHT_STRTAB:
                info_sect_strtab(sect, &sect_header, true, out_buffer);
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

    if (strlen(out_buffer) == 0)
    {
        sprintf(out_buffer, "Section %jd\n(empty)", (uintmax_t)elf_ndxscn(sect));
    }
}
