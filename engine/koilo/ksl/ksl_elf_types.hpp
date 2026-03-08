// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../registry/reflect_macros.hpp"
// Portable ELF32/ELF64 type definitions.
// No dependency on system <elf.h> - works on Linux, Windows, and MCU.

#include <cstdint>

namespace ksl {
namespace elf {

// ELF magic
static constexpr uint8_t ELFMAG0 = 0x7f;
static constexpr uint8_t ELFMAG1 = 'E';
static constexpr uint8_t ELFMAG2 = 'L';
static constexpr uint8_t ELFMAG3 = 'F';

// ELF class
static constexpr uint8_t ELFCLASS32 = 1;
static constexpr uint8_t ELFCLASS64 = 2;

// Data encoding
static constexpr uint8_t ELFDATA2LSB = 1;

// Object type
static constexpr uint16_t ET_DYN = 3;

// Machine types
static constexpr uint16_t EM_ARM    = 40;
static constexpr uint16_t EM_X86_64 = 62;

// Identification indices
static constexpr int EI_CLASS  = 4;
static constexpr int EI_DATA   = 5;
static constexpr int EI_NIDENT = 16;

// Section header types
static constexpr uint32_t SHT_DYNSYM = 11;

// Section header flags
static constexpr uint32_t SHF_ALLOC     = 0x2;
static constexpr uint32_t SHF_EXECINSTR = 0x4;

// Symbol binding
static constexpr uint8_t STB_GLOBAL = 1;
static constexpr uint8_t STB_WEAK   = 2;

// Symbol type
static constexpr uint8_t STT_FUNC   = 2;
static constexpr uint8_t STT_OBJECT = 1;

// Special section indices
static constexpr uint16_t SHN_UNDEF = 0;

// Program header types
static constexpr uint32_t PT_LOAD    = 1;
static constexpr uint32_t PT_DYNAMIC = 2;

// Dynamic section tags
static constexpr int64_t DT_NULL    = 0;
static constexpr int64_t DT_STRTAB  = 5;
static constexpr int64_t DT_SYMTAB  = 6;
static constexpr int64_t DT_RELA    = 7;
static constexpr int64_t DT_RELASZ  = 8;
static constexpr int64_t DT_REL     = 17;
static constexpr int64_t DT_RELSZ   = 18;
static constexpr int64_t DT_JMPREL  = 23;
static constexpr int64_t DT_PLTRELSZ = 2;
static constexpr int64_t DT_PLTREL  = 20;
static constexpr int64_t DT_HASH    = 4;

// ARM relocation types (dynamic)
static constexpr uint32_t R_ARM_NONE      = 0;
static constexpr uint32_t R_ARM_ABS32     = 2;
static constexpr uint32_t R_ARM_GLOB_DAT  = 21;
static constexpr uint32_t R_ARM_JUMP_SLOT = 22;
static constexpr uint32_t R_ARM_RELATIVE  = 23;

// x86_64 relocation types (dynamic)
static constexpr uint32_t R_X86_64_NONE      = 0;
static constexpr uint32_t R_X86_64_64        = 1;
static constexpr uint32_t R_X86_64_GLOB_DAT  = 6;
static constexpr uint32_t R_X86_64_JUMP_SLOT = 7;
static constexpr uint32_t R_X86_64_RELATIVE  = 8;

// --- ELF32 structures -----------------------------------------------

struct Elf32_Ehdr {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;

    KL_BEGIN_FIELDS(Elf32_Ehdr)
        KL_FIELD(Elf32_Ehdr, e_type, "E type", 0, 65535),
        KL_FIELD(Elf32_Ehdr, e_machine, "E machine", 0, 65535),
        KL_FIELD(Elf32_Ehdr, e_version, "E version", 0, 4294967295),
        KL_FIELD(Elf32_Ehdr, e_entry, "E entry", 0, 4294967295),
        KL_FIELD(Elf32_Ehdr, e_phoff, "E phoff", 0, 4294967295),
        KL_FIELD(Elf32_Ehdr, e_shoff, "E shoff", 0, 4294967295),
        KL_FIELD(Elf32_Ehdr, e_flags, "E flags", 0, 4294967295),
        KL_FIELD(Elf32_Ehdr, e_ehsize, "E ehsize", 0, 65535),
        KL_FIELD(Elf32_Ehdr, e_phentsize, "E phentsize", 0, 65535),
        KL_FIELD(Elf32_Ehdr, e_phnum, "E phnum", 0, 65535),
        KL_FIELD(Elf32_Ehdr, e_shentsize, "E shentsize", 0, 65535),
        KL_FIELD(Elf32_Ehdr, e_shnum, "E shnum", 0, 65535),
        KL_FIELD(Elf32_Ehdr, e_shstrndx, "E shstrndx", 0, 65535)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf32_Ehdr)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf32_Ehdr)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf32_Ehdr)

};

struct Elf32_Phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;

    KL_BEGIN_FIELDS(Elf32_Phdr)
        KL_FIELD(Elf32_Phdr, p_type, "P type", 0, 4294967295),
        KL_FIELD(Elf32_Phdr, p_offset, "P offset", 0, 4294967295),
        KL_FIELD(Elf32_Phdr, p_vaddr, "P vaddr", 0, 4294967295),
        KL_FIELD(Elf32_Phdr, p_paddr, "P paddr", 0, 4294967295),
        KL_FIELD(Elf32_Phdr, p_filesz, "P filesz", 0, 4294967295),
        KL_FIELD(Elf32_Phdr, p_memsz, "P memsz", 0, 4294967295),
        KL_FIELD(Elf32_Phdr, p_flags, "P flags", 0, 4294967295),
        KL_FIELD(Elf32_Phdr, p_align, "P align", 0, 4294967295)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf32_Phdr)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf32_Phdr)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf32_Phdr)

};

struct Elf32_Shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;

    KL_BEGIN_FIELDS(Elf32_Shdr)
        KL_FIELD(Elf32_Shdr, sh_name, "Sh name", 0, 4294967295),
        KL_FIELD(Elf32_Shdr, sh_type, "Sh type", 0, 4294967295),
        KL_FIELD(Elf32_Shdr, sh_flags, "Sh flags", 0, 4294967295),
        KL_FIELD(Elf32_Shdr, sh_addr, "Sh addr", 0, 4294967295),
        KL_FIELD(Elf32_Shdr, sh_offset, "Sh offset", 0, 4294967295),
        KL_FIELD(Elf32_Shdr, sh_size, "Sh size", 0, 4294967295),
        KL_FIELD(Elf32_Shdr, sh_link, "Sh link", 0, 4294967295),
        KL_FIELD(Elf32_Shdr, sh_info, "Sh info", 0, 4294967295),
        KL_FIELD(Elf32_Shdr, sh_addralign, "Sh addralign", 0, 4294967295),
        KL_FIELD(Elf32_Shdr, sh_entsize, "Sh entsize", 0, 4294967295)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf32_Shdr)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf32_Shdr)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf32_Shdr)

};

struct Elf32_Sym {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;

    KL_BEGIN_FIELDS(Elf32_Sym)
        KL_FIELD(Elf32_Sym, st_name, "St name", 0, 4294967295),
        KL_FIELD(Elf32_Sym, st_value, "St value", 0, 4294967295),
        KL_FIELD(Elf32_Sym, st_size, "St size", 0, 4294967295),
        KL_FIELD(Elf32_Sym, st_info, "St info", 0, 255),
        KL_FIELD(Elf32_Sym, st_other, "St other", 0, 255),
        KL_FIELD(Elf32_Sym, st_shndx, "St shndx", 0, 65535)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf32_Sym)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf32_Sym)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf32_Sym)

};

struct Elf32_Rel {
    uint32_t r_offset;
    uint32_t r_info;

    KL_BEGIN_FIELDS(Elf32_Rel)
        KL_FIELD(Elf32_Rel, r_offset, "R offset", 0, 4294967295),
        KL_FIELD(Elf32_Rel, r_info, "R info", 0, 4294967295)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf32_Rel)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf32_Rel)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf32_Rel)

};

struct Elf32_Rela {
    uint32_t r_offset;
    uint32_t r_info;
    int32_t  r_addend;

    KL_BEGIN_FIELDS(Elf32_Rela)
        KL_FIELD(Elf32_Rela, r_offset, "R offset", 0, 4294967295),
        KL_FIELD(Elf32_Rela, r_info, "R info", 0, 4294967295),
        KL_FIELD(Elf32_Rela, r_addend, "R addend", -2147483648, 2147483647)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf32_Rela)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf32_Rela)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf32_Rela)

};

struct Elf32_Dyn {
    int32_t  d_tag;
    uint32_t d_val;

    KL_BEGIN_FIELDS(Elf32_Dyn)
        KL_FIELD(Elf32_Dyn, d_tag, "D tag", -2147483648, 2147483647),
        KL_FIELD(Elf32_Dyn, d_val, "D val", 0, 4294967295)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf32_Dyn)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf32_Dyn)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf32_Dyn)

};

// --- ELF64 structures -----------------------------------------------

struct Elf64_Ehdr {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;

    KL_BEGIN_FIELDS(Elf64_Ehdr)
        KL_FIELD(Elf64_Ehdr, e_type, "E type", 0, 65535),
        KL_FIELD(Elf64_Ehdr, e_machine, "E machine", 0, 65535),
        KL_FIELD(Elf64_Ehdr, e_version, "E version", 0, 4294967295),
        KL_FIELD(Elf64_Ehdr, e_entry, "E entry", 0, 18446744073709551615),
        KL_FIELD(Elf64_Ehdr, e_phoff, "E phoff", 0, 18446744073709551615),
        KL_FIELD(Elf64_Ehdr, e_shoff, "E shoff", 0, 18446744073709551615),
        KL_FIELD(Elf64_Ehdr, e_flags, "E flags", 0, 4294967295),
        KL_FIELD(Elf64_Ehdr, e_ehsize, "E ehsize", 0, 65535),
        KL_FIELD(Elf64_Ehdr, e_phentsize, "E phentsize", 0, 65535),
        KL_FIELD(Elf64_Ehdr, e_phnum, "E phnum", 0, 65535),
        KL_FIELD(Elf64_Ehdr, e_shentsize, "E shentsize", 0, 65535),
        KL_FIELD(Elf64_Ehdr, e_shnum, "E shnum", 0, 65535),
        KL_FIELD(Elf64_Ehdr, e_shstrndx, "E shstrndx", 0, 65535)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf64_Ehdr)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf64_Ehdr)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf64_Ehdr)

};

struct Elf64_Phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;

    KL_BEGIN_FIELDS(Elf64_Phdr)
        KL_FIELD(Elf64_Phdr, p_type, "P type", 0, 4294967295),
        KL_FIELD(Elf64_Phdr, p_flags, "P flags", 0, 4294967295),
        KL_FIELD(Elf64_Phdr, p_offset, "P offset", 0, 18446744073709551615),
        KL_FIELD(Elf64_Phdr, p_vaddr, "P vaddr", 0, 18446744073709551615),
        KL_FIELD(Elf64_Phdr, p_paddr, "P paddr", 0, 18446744073709551615),
        KL_FIELD(Elf64_Phdr, p_filesz, "P filesz", 0, 18446744073709551615),
        KL_FIELD(Elf64_Phdr, p_memsz, "P memsz", 0, 18446744073709551615),
        KL_FIELD(Elf64_Phdr, p_align, "P align", 0, 18446744073709551615)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf64_Phdr)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf64_Phdr)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf64_Phdr)

};

struct Elf64_Shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;

    KL_BEGIN_FIELDS(Elf64_Shdr)
        KL_FIELD(Elf64_Shdr, sh_name, "Sh name", 0, 4294967295),
        KL_FIELD(Elf64_Shdr, sh_type, "Sh type", 0, 4294967295),
        KL_FIELD(Elf64_Shdr, sh_flags, "Sh flags", 0, 18446744073709551615),
        KL_FIELD(Elf64_Shdr, sh_addr, "Sh addr", 0, 18446744073709551615),
        KL_FIELD(Elf64_Shdr, sh_offset, "Sh offset", 0, 18446744073709551615),
        KL_FIELD(Elf64_Shdr, sh_size, "Sh size", 0, 18446744073709551615),
        KL_FIELD(Elf64_Shdr, sh_link, "Sh link", 0, 4294967295),
        KL_FIELD(Elf64_Shdr, sh_info, "Sh info", 0, 4294967295),
        KL_FIELD(Elf64_Shdr, sh_addralign, "Sh addralign", 0, 18446744073709551615),
        KL_FIELD(Elf64_Shdr, sh_entsize, "Sh entsize", 0, 18446744073709551615)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf64_Shdr)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf64_Shdr)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf64_Shdr)

};

struct Elf64_Sym {
    uint32_t st_name;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;

    KL_BEGIN_FIELDS(Elf64_Sym)
        KL_FIELD(Elf64_Sym, st_name, "St name", 0, 4294967295),
        KL_FIELD(Elf64_Sym, st_info, "St info", 0, 255),
        KL_FIELD(Elf64_Sym, st_other, "St other", 0, 255),
        KL_FIELD(Elf64_Sym, st_shndx, "St shndx", 0, 65535),
        KL_FIELD(Elf64_Sym, st_value, "St value", 0, 18446744073709551615),
        KL_FIELD(Elf64_Sym, st_size, "St size", 0, 18446744073709551615)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf64_Sym)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf64_Sym)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf64_Sym)

};

struct Elf64_Rela {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t  r_addend;

    KL_BEGIN_FIELDS(Elf64_Rela)
        KL_FIELD(Elf64_Rela, r_offset, "R offset", 0, 18446744073709551615),
        KL_FIELD(Elf64_Rela, r_info, "R info", 0, 18446744073709551615),
        KL_FIELD(Elf64_Rela, r_addend, "R addend", -9223372036854775808, 9223372036854775807)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf64_Rela)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf64_Rela)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf64_Rela)

};

struct Elf64_Dyn {
    int64_t  d_tag;
    uint64_t d_val;

    KL_BEGIN_FIELDS(Elf64_Dyn)
        KL_FIELD(Elf64_Dyn, d_tag, "D tag", -9223372036854775808, 9223372036854775807),
        KL_FIELD(Elf64_Dyn, d_val, "D val", 0, 18446744073709551615)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Elf64_Dyn)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Elf64_Dyn)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Elf64_Dyn)

};

// --- Helper functions ------------------------------------------------

inline uint32_t ELF32_R_SYM(uint32_t info)  { return info >> 8; }
inline uint8_t  ELF32_R_TYPE(uint32_t info)  { return static_cast<uint8_t>(info & 0xff); }
inline uint8_t  ELF32_ST_BIND(uint8_t info)  { return info >> 4; }
inline uint8_t  ELF32_ST_TYPE(uint8_t info)  { return info & 0xf; }

inline uint32_t ELF64_R_SYM(uint64_t info)  { return static_cast<uint32_t>(info >> 32); }
inline uint32_t ELF64_R_TYPE(uint64_t info)  { return static_cast<uint32_t>(info & 0xffffffff); }
inline uint8_t  ELF64_ST_BIND(uint8_t info)  { return info >> 4; }
inline uint8_t  ELF64_ST_TYPE(uint8_t info)  { return info & 0xf; }

} // namespace elf
} // namespace ksl
