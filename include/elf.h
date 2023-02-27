// Format of an ELF executable file
#pragma once
#include "types.h"

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

#define EI_MAG0 0  // e_ident[] indexes
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3

#define EI_CLASS 4
#define ELFCLASS32 1
#define ELFCLASS64 2

#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_PAD 9
#define EI_NIDENT 16

typedef uint32 elf32_addr;
typedef uint16 elf32_half;
typedef uint32 elf32_off;
typedef int32 elf32_sword;
typedef uint32 elf32_word;

typedef uint64 elf64_addr;
typedef uint16 elf64_half;
typedef uint64 elf64_off;
typedef uint32 elf64_word;
typedef int32 elf64_sword;
typedef uint64 elf64_xword;
typedef int64 elf64_sxword;

// File header
struct elf32hdr
{
	uchar elf[EI_NIDENT];
	elf32_half type;
	elf32_half machine;
	elf32_word version;
	elf32_addr entry;
	elf32_off phoff;
	elf32_off shoff;
	elf32_word flags;
	elf32_half ehsize;
	elf32_half phentsize;
	elf32_half phnum;
	elf32_half shentsize;
	elf32_half shnum;
	elf32_half shstrndx;
};

struct elf64hdr
{
	uchar elf[EI_NIDENT];
	elf64_half type;
	elf64_half machine;
	elf64_word version;
	elf64_addr entry;
	elf64_off phoff;
	elf64_off shoff;
	elf64_word flags;
	elf64_half ehsize;
	elf64_half phentsize;
	elf64_half phnum;
	elf64_half shentsize;
	elf64_half shnum;
	elf64_half shstrndx;
};

// Program section header
struct prog32hdr
{
	elf32_word type;
	elf32_off off;
	elf32_addr vaddr;
	elf32_addr paddr;
	elf32_word filesz;
	elf32_word memsz;
	uint flags;
	elf32_word align;
};

struct prog64hdr
{
	elf64_word type;
	elf64_word flags;
	elf64_off off;
	elf64_addr vaddr;
	elf64_addr paddr;
	elf64_xword filesz;
	elf64_xword memsz;
	elf64_xword align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
