#include "elfibia.h"

#include <stdlib.h>
#include <string.h>

#define CUSTOM_BUFFER_SIZE 50
static char custom_buf[CUSTOM_BUFFER_SIZE];

static char * get_seg_type(size_t seg_type)
{
    switch (seg_type)
    {
        case PT_NULL:
            return "NULL";
        case PT_LOAD:
            return "LOAD";
        case PT_DYNAMIC:
            return "DYNAMIC";
        case PT_INTERP:
            return "INTERP";
        case PT_NOTE:
            return "NOTE";
        case PT_SHLIB:
            return "SHLIB";
        case PT_PHDR:
            return "PHDR";
        case PT_TLS:
            return "TLS";
        case PT_GNU_PROPERTY:
            return "GNU_PROPERTY";
        case PT_GNU_EH_FRAME:
            return "GNU_EH_FRAME";
        case PT_GNU_STACK:
            return "GNU_STACK";
        case PT_GNU_RELRO:
            return "GNU_RELRO";
    default:
        if ((seg_type >= PT_LOOS) && (seg_type <= PT_HIOS))
        {
            sprintf(custom_buf, "OS Specific: (0x%lx)", seg_type);
        }
        else if ((seg_type >= PT_LOPROC) && (seg_type <= PT_HIPROC))
        {
            sprintf(custom_buf, "Processor Specific: (0x%lx)", seg_type);
        }
        else
        {
            sprintf(custom_buf, "<unknown>: 0x%lx", seg_type);
        }

        return custom_buf;
    }
}

void efb_get_segment_content(Elf *sElf, char * out_buffer)
{
    size_t seg_count;
    GElf_Phdr prg_hdr;

    if (elf_getphdrnum(sElf, &seg_count) != 0)
    {
        printf("elf_getphdrnum() failed: %s.\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    for (int idx = 0; idx < seg_count; idx++)
    {
        if (gelf_getphdr(sElf, idx, &prg_hdr) != &prg_hdr)
        {
            printf("getphdr() failed: %s.\n", elf_errmsg(-1));
            exit(EXIT_FAILURE);
        }

        sprintf(&out_buffer[strlen(out_buffer)], "Segment %d\n", idx);
        sprintf(&out_buffer[strlen(out_buffer)], "  p_type:   %s\n", get_seg_type(prg_hdr.p_type));
        sprintf(&out_buffer[strlen(out_buffer)], "  p_offset: %ld\n", prg_hdr.p_offset);

        sprintf(&out_buffer[strlen(out_buffer)], "  p_align:  %ld\n", prg_hdr.p_align);
        sprintf(&out_buffer[strlen(out_buffer)], "  p_filesz: %ld\n", prg_hdr.p_filesz);

        sprintf(&out_buffer[strlen(out_buffer)], "  p_flags:  0x%x", prg_hdr.p_flags);
        sprintf(&out_buffer[strlen(out_buffer)], " [");
        if (prg_hdr.p_flags & PF_X)
        {
            sprintf(&out_buffer[strlen(out_buffer)], " execute");
        }
        if (prg_hdr.p_flags & PF_R)
        {
            sprintf(&out_buffer[strlen(out_buffer)], " read");
        }
        if (prg_hdr.p_flags & PF_W)
        {
            sprintf(&out_buffer[strlen(out_buffer)], " write");
        }
        sprintf(&out_buffer[strlen(out_buffer)], " ]\n");

        sprintf(&out_buffer[strlen(out_buffer)], "  p_memsz:  %ld\n", prg_hdr.p_memsz);
        sprintf(&out_buffer[strlen(out_buffer)], "  p_paddr:  0x%lx\n", prg_hdr.p_paddr);
        sprintf(&out_buffer[strlen(out_buffer)], "  p_vaddr:  0x%lx\n\n", prg_hdr.p_vaddr);
    }
}
