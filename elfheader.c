#include "elfibia.h"

//#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>

void efb_get_elf_header(efb_context *efb_ctx, char * out_buffer)
{
    int elf_class;
    char * elf_ident;
    GElf_Ehdr elf_hdr;

    if (gelf_getehdr(efb_ctx->sElf, &elf_hdr) == NULL)
    {
        printf("gelf_getehdr() failed: %s.\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    sprintf(&out_buffer[strlen(out_buffer)], "ELF Header:\n");

    if ((elf_ident = elf_getident(efb_ctx->sElf, NULL)) == NULL)
    {
        printf("elf_getident() failed: %s.\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    sprintf(&out_buffer[strlen(out_buffer)], "  Magic:                             ");

    for (int idx = 0; idx < EI_NIDENT; idx++)
    {
        sprintf(&out_buffer[strlen(out_buffer)], "%02X ", elf_ident[idx]);
    }

    sprintf(&out_buffer[strlen(out_buffer)], "\n");

    if ((elf_class = gelf_getclass(efb_ctx->sElf)) == ELFCLASSNONE)
    {
        printf("gelf_getclass() failed: %s.\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    sprintf(&out_buffer[strlen(out_buffer)], "  Class:                             ELF%d\n", elf_class == ELFCLASS32 ? 32 : 64);

    sprintf(&out_buffer[strlen(out_buffer)], "  Data:                              %s\n", "FIXME");
    sprintf(&out_buffer[strlen(out_buffer)], "  Version:                           %s\n", "FIXME");
    sprintf(&out_buffer[strlen(out_buffer)], "  OS/ABI:                            %s\n", "FIXME");
    sprintf(&out_buffer[strlen(out_buffer)], "  ABI Version:                       %s\n", "FIXME");
    sprintf(&out_buffer[strlen(out_buffer)], "  Type:                              (%d) %s\n", elf_hdr.e_type, "FIXME");
    sprintf(&out_buffer[strlen(out_buffer)], "  Machine:                           (%d) %s\n", elf_hdr.e_machine, "FIXME");
    sprintf(&out_buffer[strlen(out_buffer)], "  Version:                           0x%x\n", elf_hdr.e_version);
    sprintf(&out_buffer[strlen(out_buffer)], "  Entry point address:               0x%lx\n", elf_hdr.e_entry);
    sprintf(&out_buffer[strlen(out_buffer)], "  Start of program headers:          %lu (bytes into file)\n", elf_hdr.e_phoff);
    sprintf(&out_buffer[strlen(out_buffer)], "  Start of section headers:          %lu (bytes into file)\n", elf_hdr.e_shoff);
    sprintf(&out_buffer[strlen(out_buffer)], "  Flags:                             0x%x\n", elf_hdr.e_flags);
    sprintf(&out_buffer[strlen(out_buffer)], "  Size of this header:               %d (bytes)\n", elf_hdr.e_ehsize);
    sprintf(&out_buffer[strlen(out_buffer)], "  Size of program headers:           %d (bytes)\n", elf_hdr.e_phentsize);
    sprintf(&out_buffer[strlen(out_buffer)], "  Number of program headers:         %d\n", elf_hdr.e_phnum);
    sprintf(&out_buffer[strlen(out_buffer)], "  Size of section headers:           %d (bytes)\n", elf_hdr.e_shentsize);
    sprintf(&out_buffer[strlen(out_buffer)], "  Number of section headers:         %d\n", elf_hdr.e_shnum);
    sprintf(&out_buffer[strlen(out_buffer)], "  Section header string table index: %d\n", elf_hdr.e_shstrndx);
}
