/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* Partly copied from the Linux kernel */

/* 64-bit ELF base types. */
typedef __u64 Elf64_Addr;
typedef __u16 Elf64_Half;
typedef __s16 Elf64_SHalf;
typedef __u64 Elf64_Off;
typedef __s32 Elf64_Sword;
typedef __u32 Elf64_Word;
typedef __u64 Elf64_Xword;
typedef __s64 Elf64_Sxword;

#define EI_NIDENT	16

#define ELFMAG		"\177ELF"
#define SELFMAG		4

#define PT_LOAD		1

#define EM_AARCH64	183 /* ARM 64 bit */
#define EM_RISCV	243 /* RISC-V */

typedef struct elf64_hdr {
	unsigned char e_ident[EI_NIDENT];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off e_phoff;
	Elf64_Off e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
} Elf64_Ehdr;   

#define PF_R		0x4
#define PF_W		0x2
#define PF_X		0x1

typedef struct elf64_phdr {
	Elf64_Word p_type;
	Elf64_Word p_flags;
	Elf64_Off p_offset;           /* Segment file offset */
	Elf64_Addr p_vaddr;           /* Segment virtual address */
	Elf64_Addr p_paddr;           /* Segment physical address */
	Elf64_Xword p_filesz;         /* Segment size in file */
	Elf64_Xword p_memsz;          /* Segment size in memory */
	Elf64_Xword p_align;          /* Segment alignment, file & memory */
} Elf64_Phdr;

