#include "elfibia.h"

#include <stdlib.h>
#include <string.h>

#define CUSTOM_BUFFER_SIZE 50
char custom_buf[CUSTOM_BUFFER_SIZE];

static char * get_elf_data(const int elf_data)
{
    switch (elf_data)
    {
    case ELFDATANONE:
        return "none";
        break;
    case ELFDATA2LSB:
        return "2's complement, little endian";
        break;
    case ELFDATA2MSB:
        return "2's complement, big endian";
        break;
    default:
        sprintf(custom_buf, "<unknown: %x>", elf_data);
        return custom_buf;
        break;
    }
}

static char * get_elf_version(const int elf_version)
{
    sprintf(custom_buf, "%d%s", elf_version, (elf_version == EV_CURRENT ? " (current)" : (elf_version != EV_NONE ? " <unknown>" : "")));
    return custom_buf;
}

static char * get_elf_ssabi(unsigned int elf_osabi)
{
    switch (elf_osabi)
    {
    case ELFOSABI_NONE:
        return "UNIX - System V";
        break;
    case ELFOSABI_HPUX:
        return "UNIX - HP-UX";
        break;
    case ELFOSABI_NETBSD:
        return "UNIX - NetBSD";
        break;
    case ELFOSABI_GNU:
        return "UNIX - GNU";
        break;
    case ELFOSABI_SOLARIS:
        return "UNIX - Solaris";
        break;
    case ELFOSABI_AIX:
        return "UNIX - AIX";
        break;
    case ELFOSABI_IRIX:
        return "UNIX - IRIX";
        break;
    case ELFOSABI_FREEBSD:
        return "UNIX - FreeBSD";
        break;
    case ELFOSABI_TRU64:
        return "UNIX - TRU64";
        break;
    case ELFOSABI_MODESTO:
        return "Novell - Modesto";
        break;
    case ELFOSABI_OPENBSD:
        return "UNIX - OpenBSD";
        break;
    default:
        sprintf(custom_buf, "Not set in elfibia (%d)", elf_osabi);
        return custom_buf;
        break;
    }
}

static char * get_elf_type(const int elf_type)
{
    switch (elf_type)
    {
    case ET_NONE:
        return "NONE (None)";
        break;
    case ET_REL:
        return "REL (Relocatable file)";
        break;
    case ET_EXEC:
        return "EXEC (Executable file)";
        break;
    case ET_DYN:
        return "DYN (TODO: return the correct DYN type - PIE or SO)";
        break;
// TODO get the correct DYN type (as it is done by the readelf app)
//    case ET_DYN:
//        return "DYN (Position-Independent Executable file)";
//        break;
//    case ET_DYN:
//        return "DYN (Shared object file)";
//        break;
    case ET_CORE:
        return "CORE (Core file)";
        break;
    default:
        if ((elf_type >= ET_LOOS) && (elf_type <= ET_HIOS))
        {
            sprintf(custom_buf, "OS Specific: (%x)", elf_type);
        }
        else if ((elf_type >= ET_LOPROC) && (elf_type <= ET_HIPROC))
        {
            sprintf(custom_buf, "Processor Specific: (%x)", elf_type);
        }
        else
        {
            sprintf(custom_buf, "<unknown>: %x", elf_type);
        }

        return custom_buf;
        break;
    }
}

static char * get_machine(const int machine)
{
    switch (machine)
    {
    case EM_NONE:
        return "No machine";
        break;
    case EM_386:
        return "Intel 80386";
        break;
    case EM_PPC64:
        return "PowerPC64";
        break;
    case EM_S390:
        return "IBM S/390";
        break;
    case EM_ARM:
        return "ARM";
        break;
    case EM_X86_64:
        return "Advanced Micro Devices X86-64";
        break;
    case EM_AARCH64:
        return "AArch64";
        break;
    case EM_RISCV:
        return "RISC-V";
        break;
    default:
        sprintf(custom_buf, "Not set in elfibia (%d)", machine);
        return custom_buf;
        break;
    }
}

void efb_get_elf_header(efb_context const * const efb_ctx, char * out_buffer)
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

    sprintf(&out_buffer[strlen(out_buffer)], "  Data:                              %s\n", get_elf_data(elf_ident[EI_DATA]));
    sprintf(&out_buffer[strlen(out_buffer)], "  Version:                           %s\n", get_elf_version(elf_ident[EI_VERSION]));
    sprintf(&out_buffer[strlen(out_buffer)], "  OS/ABI:                            %s\n", get_elf_ssabi(elf_ident[EI_OSABI]));
    sprintf(&out_buffer[strlen(out_buffer)], "  ABI Version:                       %d\n", elf_ident[EI_ABIVERSION]);
    sprintf(&out_buffer[strlen(out_buffer)], "  Type:                              %s\n", get_elf_type(elf_hdr.e_type));
    sprintf(&out_buffer[strlen(out_buffer)], "  Machine:                           %s\n", get_machine(elf_hdr.e_machine));
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
