/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define ELF_ARCH	EM_AARCH64

/* The only relocation a position independent image leaves for the loader */
#define ELF_R_RELATIVE	1027 /* R_AARCH64_RELATIVE */

typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
typedef Elf64_Dyn Elf_Dyn;
typedef Elf64_Rela Elf_Rela;
#define ELF_R_TYPE	ELF64_R_TYPE
