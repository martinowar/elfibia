#include "elfibia.h"

#include <elf.h>
#include <stdlib.h>

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
