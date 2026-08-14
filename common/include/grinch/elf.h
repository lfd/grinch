/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* Partly copied from the Linux kernel */

#ifndef _ELF_H
#define _ELF_H

#include <asm-generic/elf.h>
#include <grinch/types.h>

/* 32-bit ELF base types. */
typedef __u32   Elf32_Addr;
typedef __u16   Elf32_Half;
typedef __u32   Elf32_Off;
typedef __s32   Elf32_Sword;
typedef __u32   Elf32_Word;

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

#define ET_EXEC		2
#define ET_DYN		3

#define PT_LOAD		1
#define PT_DYNAMIC	2

/* Where the relocations are, and how much of them */
#define DT_NULL		0
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9

/* A relocation the linker dropped, left behind to keep the table its size */
#define ELF_R_NONE	0

#define ELF32_R_TYPE(i)	((i) & 0xff)
#define ELF64_R_TYPE(i)	((i) & 0xffffffff)

#define EM_AARCH64	183 /* ARM 64 bit */
#define EM_RISCV	243 /* RISC-V */

typedef struct elf32_hdr {
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half    e_type;
	Elf32_Half    e_machine;
	Elf32_Word    e_version;
	Elf32_Addr    e_entry;  /* Entry point */
	Elf32_Off     e_phoff;
	Elf32_Off     e_shoff;
	Elf32_Word    e_flags;
	Elf32_Half    e_ehsize;
	Elf32_Half    e_phentsize;
	Elf32_Half    e_phnum;
	Elf32_Half    e_shentsize;
	Elf32_Half    e_shnum;
	Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

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


typedef struct elf32_phdr {
	Elf32_Word p_type;
	Elf32_Off p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Word p_filesz;
	Elf32_Word p_memsz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
} Elf32_Phdr;

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

typedef struct elf32_dyn {
	Elf32_Sword d_tag;
	union {
		Elf32_Word d_val;
		Elf32_Addr d_ptr;
	} d_un;
} Elf32_Dyn;

typedef struct elf64_dyn {
	Elf64_Sxword d_tag;
	union {
		Elf64_Xword d_val;
		Elf64_Addr d_ptr;
	} d_un;
} Elf64_Dyn;

typedef struct elf32_rela {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
	Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct elf64_rela {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
	Elf64_Sxword r_addend;
} Elf64_Rela;

#define AT_NULL		0 /* End of vector */
#define AT_KINFO	100 /* Address if struct kinfo */

#include <asm/elf.h>

#endif /* _ELF_H */
