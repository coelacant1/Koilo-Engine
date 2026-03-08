// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Unified ELF loader for KSL shader modules.
// Replaces dlopen/LoadLibrary with a portable ELF parser that works
// identically on Linux, Windows (via Clang-compiled ELF), and MCU.
//
// Supports:
//   - ELF32 (ARM Cortex-M7 for Teensy) and ELF64 (x86_64 for desktop)
//   - ET_DYN shared objects (compiled with -shared -fpic)
//   - Dynamic relocations: RELATIVE, GLOB_DAT, JUMP_SLOT, ABS32/64
//   - Symbol resolution against an engine-provided symbol table

#include "ksl_elf_types.hpp"
#include "ksl_memory.hpp"
#include "ksl_symbols.hpp"

#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <vector>

namespace ksl {

class KSLElfLoader {
public:
    KSLElfLoader() = default;
    ~KSLElfLoader() { Unload(); }

    KSLElfLoader(const KSLElfLoader&) = delete;
    KSLElfLoader& operator=(const KSLElfLoader&) = delete;

    KSLElfLoader(KSLElfLoader&& o) noexcept
        : base_(o.base_), allocSize_(o.allocSize_), exports_(std::move(o.exports_)) {
        std::memcpy(error_, o.error_, sizeof(error_));
        o.base_ = nullptr;
        o.allocSize_ = 0;
    }

    KSLElfLoader& operator=(KSLElfLoader&& o) noexcept {
        if (this != &o) {
            Unload();
            base_ = o.base_;
            allocSize_ = o.allocSize_;
            exports_ = std::move(o.exports_);
            std::memcpy(error_, o.error_, sizeof(error_));
            o.base_ = nullptr;
            o.allocSize_ = 0;
        }
        return *this;
    }

    // Load an ELF shared object from raw file bytes.
    // Allocates executable memory, processes relocations, resolves symbols.
    bool Load(const uint8_t* data, size_t dataSize, const KSLSymbolTable& imports) {
        if (base_) Unload();

        if (dataSize < elf::EI_NIDENT) {
            SetError("File too small for ELF header");
            return false;
        }

        // Validate magic
        if (data[0] != elf::ELFMAG0 || data[1] != elf::ELFMAG1 ||
            data[2] != elf::ELFMAG2 || data[3] != elf::ELFMAG3) {
            SetError("Not an ELF file (bad magic)");
            return false;
        }

        // Must be little-endian
        if (data[elf::EI_DATA] != elf::ELFDATA2LSB) {
            SetError("Only little-endian ELF supported");
            return false;
        }

        uint8_t elfClass = data[elf::EI_CLASS];
        if (elfClass == elf::ELFCLASS64)
            return LoadDyn64(data, dataSize, imports);
        if (elfClass == elf::ELFCLASS32)
            return LoadDyn32(data, dataSize, imports);

        SetError("Unsupported ELF class: %u", elfClass);
        return false;
    }

    // Find an exported symbol by name. Returns nullptr if not found.
    void* FindExport(const char* name) const {
        for (const auto& e : exports_) {
            if (std::strcmp(e.name, name) == 0)
                return e.addr;
        }
        return nullptr;
    }

    // Typed symbol lookup.
    template<typename T>
    T GetSymbol(const char* name) const {
        return reinterpret_cast<T>(FindExport(name));
    }

    void Unload() {
        if (base_) {
            FreeExecutable(base_, allocSize_);
            base_ = nullptr;
            allocSize_ = 0;
        }
        exports_.clear();
        error_[0] = '\0';
    }

    bool IsLoaded() const { return base_ != nullptr; }
    const char* GetError() const { return error_; }

private:
    uint8_t* base_ = nullptr;
    size_t allocSize_ = 0;

    struct ExportEntry {
        char name[64];
        void* addr;
    };
    std::vector<ExportEntry> exports_;
    char error_[512] = {};

    void SetError(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(error_, sizeof(error_), fmt, args);
        va_end(args);
    }

    // --- ELF64 loading (x86_64 desktop) ------------------------------

    bool LoadDyn64(const uint8_t* data, size_t fileSize,
                   const KSLSymbolTable& imports) {
        if (fileSize < sizeof(elf::Elf64_Ehdr)) {
            SetError("File too small for ELF64 header");
            return false;
        }

        auto* ehdr = reinterpret_cast<const elf::Elf64_Ehdr*>(data);
        if (ehdr->e_type != elf::ET_DYN) {
            SetError("Not a shared object (type=%u)", ehdr->e_type);
            return false;
        }

        // Validate program headers exist
        if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
            SetError("No program headers");
            return false;
        }

        auto* phdrs = reinterpret_cast<const elf::Elf64_Phdr*>(data + ehdr->e_phoff);

        // Find virtual address range of all LOAD segments
        uint64_t minVaddr = UINT64_MAX, maxVaddr = 0;
        for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
            if (phdrs[i].p_type == elf::PT_LOAD) {
                if (phdrs[i].p_vaddr < minVaddr) minVaddr = phdrs[i].p_vaddr;
                uint64_t end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
                if (end > maxVaddr) maxVaddr = end;
            }
        }

        if (minVaddr >= maxVaddr) {
            SetError("No loadable segments");
            return false;
        }

        // Allocate executable memory
        allocSize_ = static_cast<size_t>(maxVaddr - minVaddr);
        base_ = static_cast<uint8_t*>(AllocExecutable(allocSize_));
        if (!base_) {
            SetError("Failed to allocate %zu bytes of executable memory", allocSize_);
            allocSize_ = 0;
            return false;
        }
        std::memset(base_, 0, allocSize_);

        uintptr_t bias = reinterpret_cast<uintptr_t>(base_) - static_cast<uintptr_t>(minVaddr);

        // Load segments
        for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
            if (phdrs[i].p_type != elf::PT_LOAD) continue;
            if (phdrs[i].p_offset + phdrs[i].p_filesz > fileSize) {
                SetError("Segment %u extends beyond file", i);
                Unload();
                return false;
            }
            auto* dest = reinterpret_cast<uint8_t*>(bias + phdrs[i].p_vaddr);
            std::memcpy(dest, data + phdrs[i].p_offset,
                        static_cast<size_t>(phdrs[i].p_filesz));
        }

        // Find dynamic section
        const elf::Elf64_Dyn* dynamic = nullptr;
        for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
            if (phdrs[i].p_type == elf::PT_DYNAMIC) {
                dynamic = reinterpret_cast<const elf::Elf64_Dyn*>(
                    reinterpret_cast<uint8_t*>(bias + phdrs[i].p_vaddr));
                break;
            }
        }
        if (!dynamic) {
            SetError("No dynamic section");
            Unload();
            return false;
        }

        // Parse dynamic entries
        const elf::Elf64_Sym* symtab = nullptr;
        const char* strtab = nullptr;
        const elf::Elf64_Rela* rela = nullptr;
        size_t relaCount = 0;
        const elf::Elf64_Rela* jmprel = nullptr;
        size_t jmprelSize = 0;
        const uint32_t* hashTable = nullptr;

        for (auto* d = dynamic; d->d_tag != elf::DT_NULL; d++) {
            switch (d->d_tag) {
                case elf::DT_SYMTAB:
                    symtab = reinterpret_cast<const elf::Elf64_Sym*>(bias + d->d_val);
                    break;
                case elf::DT_STRTAB:
                    strtab = reinterpret_cast<const char*>(bias + d->d_val);
                    break;
                case elf::DT_RELA:
                    rela = reinterpret_cast<const elf::Elf64_Rela*>(bias + d->d_val);
                    break;
                case elf::DT_RELASZ:
                    relaCount = static_cast<size_t>(d->d_val) / sizeof(elf::Elf64_Rela);
                    break;
                case elf::DT_JMPREL:
                    jmprel = reinterpret_cast<const elf::Elf64_Rela*>(bias + d->d_val);
                    break;
                case elf::DT_PLTRELSZ:
                    jmprelSize = static_cast<size_t>(d->d_val);
                    break;
                case elf::DT_HASH:
                    hashTable = reinterpret_cast<const uint32_t*>(bias + d->d_val);
                    break;
                default:
                    break;
            }
        }

        if (!symtab || !strtab) {
            SetError("Missing dynamic symbol or string table");
            Unload();
            return false;
        }

        // Process .rela.dyn relocations
        if (rela && relaCount > 0) {
            for (size_t i = 0; i < relaCount; i++) {
                if (!ProcessRela64(rela[i], bias, symtab, strtab, imports)) {
                    Unload();
                    return false;
                }
            }
        }

        // Process .rela.plt relocations
        if (jmprel && jmprelSize > 0) {
            size_t jmpCount = jmprelSize / sizeof(elf::Elf64_Rela);
            for (size_t i = 0; i < jmpCount; i++) {
                if (!ProcessRela64(jmprel[i], bias, symtab, strtab, imports)) {
                    Unload();
                    return false;
                }
            }
        }

        // Collect exported symbols
        size_t symCount = 0;
        if (hashTable) {
            symCount = hashTable[1]; // nchain = number of symbols
        } else {
            symCount = FindDynsymCount64(data, fileSize, ehdr);
        }

        CollectExports64(symtab, strtab, symCount, bias);
        return true;
    }

    bool ProcessRela64(const elf::Elf64_Rela& r, uintptr_t bias,
                       const elf::Elf64_Sym* symtab, const char* strtab,
                       const KSLSymbolTable& imports) {
        uint32_t type = elf::ELF64_R_TYPE(r.r_info);
        uint32_t symIdx = elf::ELF64_R_SYM(r.r_info);
        auto* target = reinterpret_cast<uint64_t*>(bias + r.r_offset);

        switch (type) {
            case elf::R_X86_64_NONE:
                break;

            case elf::R_X86_64_RELATIVE:
                *target = static_cast<uint64_t>(bias) + static_cast<uint64_t>(r.r_addend);
                break;

            case elf::R_X86_64_GLOB_DAT:
            case elf::R_X86_64_JUMP_SLOT: {
                const auto& sym = symtab[symIdx];
                void* addr = nullptr;
                if (sym.st_shndx != elf::SHN_UNDEF) {
                    addr = reinterpret_cast<void*>(bias + sym.st_value);
                } else {
                    const char* name = strtab + sym.st_name;
                    addr = imports.Resolve(name);
                    if (!addr && elf::ELF64_ST_BIND(sym.st_info) != elf::STB_WEAK) {
                        SetError("Unresolved symbol: %s", name);
                        return false;
                    }
                }
                *target = reinterpret_cast<uint64_t>(addr);
                break;
            }

            case elf::R_X86_64_64: {
                const auto& sym = symtab[symIdx];
                void* addr = nullptr;
                if (sym.st_shndx != elf::SHN_UNDEF) {
                    addr = reinterpret_cast<void*>(bias + sym.st_value);
                } else if (symIdx != 0) {
                    const char* name = strtab + sym.st_name;
                    addr = imports.Resolve(name);
                    if (!addr && elf::ELF64_ST_BIND(sym.st_info) != elf::STB_WEAK) {
                        SetError("Unresolved symbol: %s", name);
                        return false;
                    }
                }
                *target = reinterpret_cast<uint64_t>(addr) +
                          static_cast<uint64_t>(r.r_addend);
                break;
            }

            default:
                SetError("Unsupported x86_64 relocation type: %u", type);
                return false;
        }
        return true;
    }

    size_t FindDynsymCount64(const uint8_t* data, size_t fileSize,
                             const elf::Elf64_Ehdr* ehdr) {
        if (ehdr->e_shoff == 0 || ehdr->e_shnum == 0) return 0;
        if (ehdr->e_shoff + ehdr->e_shnum * sizeof(elf::Elf64_Shdr) > fileSize)
            return 0;

        auto* shdrs = reinterpret_cast<const elf::Elf64_Shdr*>(data + ehdr->e_shoff);
        for (uint16_t i = 0; i < ehdr->e_shnum; i++) {
            if (shdrs[i].sh_type == elf::SHT_DYNSYM && shdrs[i].sh_entsize > 0) {
                return static_cast<size_t>(shdrs[i].sh_size / shdrs[i].sh_entsize);
            }
        }
        return 0;
    }

    void CollectExports64(const elf::Elf64_Sym* symtab, const char* strtab,
                          size_t symCount, uintptr_t bias) {
        for (size_t i = 1; i < symCount; i++) {
            const auto& sym = symtab[i];
            uint8_t bind = elf::ELF64_ST_BIND(sym.st_info);
            uint8_t type = elf::ELF64_ST_TYPE(sym.st_info);

            if ((bind == elf::STB_GLOBAL || bind == elf::STB_WEAK) &&
                (type == elf::STT_FUNC || type == elf::STT_OBJECT) &&
                sym.st_shndx != elf::SHN_UNDEF) {

                ExportEntry entry{};
                std::strncpy(entry.name, strtab + sym.st_name, sizeof(entry.name) - 1);
                entry.addr = reinterpret_cast<void*>(bias + sym.st_value);
                exports_.push_back(entry);
            }
        }
    }

    // --- ELF32 loading (ARM Cortex-M7 for Teensy) -------------------

    bool LoadDyn32(const uint8_t* data, size_t fileSize,
                   const KSLSymbolTable& imports) {
        if (fileSize < sizeof(elf::Elf32_Ehdr)) {
            SetError("File too small for ELF32 header");
            return false;
        }

        auto* ehdr = reinterpret_cast<const elf::Elf32_Ehdr*>(data);
        if (ehdr->e_type != elf::ET_DYN) {
            SetError("Not a shared object (type=%u)", ehdr->e_type);
            return false;
        }

        if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
            SetError("No program headers");
            return false;
        }

        auto* phdrs = reinterpret_cast<const elf::Elf32_Phdr*>(data + ehdr->e_phoff);

        // Find virtual address range
        uint32_t minVaddr = UINT32_MAX, maxVaddr = 0;
        for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
            if (phdrs[i].p_type == elf::PT_LOAD) {
                if (phdrs[i].p_vaddr < minVaddr) minVaddr = phdrs[i].p_vaddr;
                uint32_t end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
                if (end > maxVaddr) maxVaddr = end;
            }
        }

        if (minVaddr >= maxVaddr) {
            SetError("No loadable segments");
            return false;
        }

        allocSize_ = static_cast<size_t>(maxVaddr - minVaddr);
        base_ = static_cast<uint8_t*>(AllocExecutable(allocSize_));
        if (!base_) {
            SetError("Failed to allocate %zu bytes of executable memory", allocSize_);
            allocSize_ = 0;
            return false;
        }
        std::memset(base_, 0, allocSize_);

        uintptr_t bias = reinterpret_cast<uintptr_t>(base_) - static_cast<uintptr_t>(minVaddr);

        // Load segments
        for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
            if (phdrs[i].p_type != elf::PT_LOAD) continue;
            if (phdrs[i].p_offset + phdrs[i].p_filesz > fileSize) {
                SetError("Segment %u extends beyond file", i);
                Unload();
                return false;
            }
            auto* dest = reinterpret_cast<uint8_t*>(bias + phdrs[i].p_vaddr);
            std::memcpy(dest, data + phdrs[i].p_offset,
                        static_cast<size_t>(phdrs[i].p_filesz));
        }

        // Find dynamic section
        const elf::Elf32_Dyn* dynamic = nullptr;
        for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
            if (phdrs[i].p_type == elf::PT_DYNAMIC) {
                dynamic = reinterpret_cast<const elf::Elf32_Dyn*>(
                    reinterpret_cast<uint8_t*>(bias + phdrs[i].p_vaddr));
                break;
            }
        }
        if (!dynamic) {
            SetError("No dynamic section");
            Unload();
            return false;
        }

        // Parse dynamic entries
        const elf::Elf32_Sym* symtab = nullptr;
        const char* strtab = nullptr;
        const elf::Elf32_Rel* rel = nullptr;
        size_t relCount = 0;
        const elf::Elf32_Rela* rela = nullptr;
        size_t relaCount = 0;
        const uint8_t* jmprel = nullptr;
        size_t jmprelSize = 0;
        int32_t pltRelType = 0;
        const uint32_t* hashTable = nullptr;

        for (auto* d = dynamic; d->d_tag != static_cast<int32_t>(elf::DT_NULL); d++) {
            switch (static_cast<int64_t>(d->d_tag)) {
                case elf::DT_SYMTAB:
                    symtab = reinterpret_cast<const elf::Elf32_Sym*>(bias + d->d_val);
                    break;
                case elf::DT_STRTAB:
                    strtab = reinterpret_cast<const char*>(bias + d->d_val);
                    break;
                case elf::DT_REL:
                    rel = reinterpret_cast<const elf::Elf32_Rel*>(bias + d->d_val);
                    break;
                case elf::DT_RELSZ:
                    relCount = d->d_val / sizeof(elf::Elf32_Rel);
                    break;
                case elf::DT_RELA:
                    rela = reinterpret_cast<const elf::Elf32_Rela*>(bias + d->d_val);
                    break;
                case elf::DT_RELASZ:
                    relaCount = d->d_val / sizeof(elf::Elf32_Rela);
                    break;
                case elf::DT_JMPREL:
                    jmprel = reinterpret_cast<const uint8_t*>(bias + d->d_val);
                    break;
                case elf::DT_PLTRELSZ:
                    jmprelSize = d->d_val;
                    break;
                case elf::DT_PLTREL:
                    pltRelType = static_cast<int32_t>(d->d_val);
                    break;
                case elf::DT_HASH:
                    hashTable = reinterpret_cast<const uint32_t*>(bias + d->d_val);
                    break;
                default:
                    break;
            }
        }

        if (!symtab || !strtab) {
            SetError("Missing dynamic symbol or string table");
            Unload();
            return false;
        }

        // Process .rel.dyn (REL format - implicit addends, typical for ARM)
        if (rel && relCount > 0) {
            for (size_t i = 0; i < relCount; i++) {
                if (!ProcessRel32(rel[i], bias, symtab, strtab, imports)) {
                    Unload();
                    return false;
                }
            }
        }

        // Process .rela.dyn (RELA format - some ARM toolchains use this)
        if (rela && relaCount > 0) {
            for (size_t i = 0; i < relaCount; i++) {
                if (!ProcessRela32(rela[i], bias, symtab, strtab, imports)) {
                    Unload();
                    return false;
                }
            }
        }

        // Process .rel.plt / .rela.plt
        if (jmprel && jmprelSize > 0) {
            if (pltRelType == static_cast<int32_t>(elf::DT_RELA)) {
                auto* entries = reinterpret_cast<const elf::Elf32_Rela*>(jmprel);
                size_t count = jmprelSize / sizeof(elf::Elf32_Rela);
                for (size_t i = 0; i < count; i++) {
                    if (!ProcessRela32(entries[i], bias, symtab, strtab, imports)) {
                        Unload();
                        return false;
                    }
                }
            } else {
                auto* entries = reinterpret_cast<const elf::Elf32_Rel*>(jmprel);
                size_t count = jmprelSize / sizeof(elf::Elf32_Rel);
                for (size_t i = 0; i < count; i++) {
                    if (!ProcessRel32(entries[i], bias, symtab, strtab, imports)) {
                        Unload();
                        return false;
                    }
                }
            }
        }

        // Collect exported symbols
        size_t symCount = 0;
        if (hashTable) {
            symCount = hashTable[1];
        } else {
            symCount = FindDynsymCount32(data, fileSize, ehdr);
        }

        CollectExports32(symtab, strtab, symCount, bias);
        return true;
    }

    bool ProcessRel32(const elf::Elf32_Rel& r, uintptr_t bias,
                      const elf::Elf32_Sym* symtab, const char* strtab,
                      const KSLSymbolTable& imports) {
        uint8_t type = elf::ELF32_R_TYPE(r.r_info);
        uint32_t symIdx = elf::ELF32_R_SYM(r.r_info);
        auto* target = reinterpret_cast<uint32_t*>(bias + r.r_offset);

        switch (type) {
            case elf::R_ARM_NONE:
                break;

            case elf::R_ARM_RELATIVE:
                *target += static_cast<uint32_t>(bias);
                break;

            case elf::R_ARM_GLOB_DAT:
            case elf::R_ARM_JUMP_SLOT: {
                const auto& sym = symtab[symIdx];
                void* addr = nullptr;
                if (sym.st_shndx != elf::SHN_UNDEF) {
                    addr = reinterpret_cast<void*>(
                        static_cast<uintptr_t>(bias + sym.st_value));
                } else {
                    const char* name = strtab + sym.st_name;
                    addr = imports.Resolve(name);
                    if (!addr && elf::ELF32_ST_BIND(sym.st_info) != elf::STB_WEAK) {
                        SetError("Unresolved symbol: %s", name);
                        return false;
                    }
                }
                *target = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(addr));
                break;
            }

            case elf::R_ARM_ABS32: {
                const auto& sym = symtab[symIdx];
                void* addr = nullptr;
                if (sym.st_shndx != elf::SHN_UNDEF) {
                    addr = reinterpret_cast<void*>(
                        static_cast<uintptr_t>(bias + sym.st_value));
                } else if (symIdx != 0) {
                    const char* name = strtab + sym.st_name;
                    addr = imports.Resolve(name);
                    if (!addr && elf::ELF32_ST_BIND(sym.st_info) != elf::STB_WEAK) {
                        SetError("Unresolved symbol: %s", name);
                        return false;
                    }
                }
                *target += static_cast<uint32_t>(reinterpret_cast<uintptr_t>(addr));
                break;
            }

            default:
                SetError("Unsupported ARM relocation type: %u", type);
                return false;
        }
        return true;
    }

    bool ProcessRela32(const elf::Elf32_Rela& r, uintptr_t bias,
                       const elf::Elf32_Sym* symtab, const char* strtab,
                       const KSLSymbolTable& imports) {
        uint8_t type = elf::ELF32_R_TYPE(r.r_info);
        uint32_t symIdx = elf::ELF32_R_SYM(r.r_info);
        auto* target = reinterpret_cast<uint32_t*>(bias + r.r_offset);

        switch (type) {
            case elf::R_ARM_NONE:
                break;

            case elf::R_ARM_RELATIVE:
                *target = static_cast<uint32_t>(bias) +
                          static_cast<uint32_t>(r.r_addend);
                break;

            case elf::R_ARM_GLOB_DAT:
            case elf::R_ARM_JUMP_SLOT: {
                const auto& sym = symtab[symIdx];
                void* addr = nullptr;
                if (sym.st_shndx != elf::SHN_UNDEF) {
                    addr = reinterpret_cast<void*>(
                        static_cast<uintptr_t>(bias + sym.st_value));
                } else {
                    const char* name = strtab + sym.st_name;
                    addr = imports.Resolve(name);
                    if (!addr && elf::ELF32_ST_BIND(sym.st_info) != elf::STB_WEAK) {
                        SetError("Unresolved symbol: %s", name);
                        return false;
                    }
                }
                *target = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(addr));
                break;
            }

            case elf::R_ARM_ABS32: {
                const auto& sym = symtab[symIdx];
                void* addr = nullptr;
                if (sym.st_shndx != elf::SHN_UNDEF) {
                    addr = reinterpret_cast<void*>(
                        static_cast<uintptr_t>(bias + sym.st_value));
                } else if (symIdx != 0) {
                    const char* name = strtab + sym.st_name;
                    addr = imports.Resolve(name);
                    if (!addr && elf::ELF32_ST_BIND(sym.st_info) != elf::STB_WEAK) {
                        SetError("Unresolved symbol: %s", name);
                        return false;
                    }
                }
                *target = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(addr)) +
                          static_cast<uint32_t>(r.r_addend);
                break;
            }

            default:
                SetError("Unsupported ARM relocation type: %u", type);
                return false;
        }
        return true;
    }

    size_t FindDynsymCount32(const uint8_t* data, size_t fileSize,
                             const elf::Elf32_Ehdr* ehdr) {
        if (ehdr->e_shoff == 0 || ehdr->e_shnum == 0) return 0;
        if (ehdr->e_shoff + ehdr->e_shnum * sizeof(elf::Elf32_Shdr) > fileSize)
            return 0;

        auto* shdrs = reinterpret_cast<const elf::Elf32_Shdr*>(data + ehdr->e_shoff);
        for (uint16_t i = 0; i < ehdr->e_shnum; i++) {
            if (shdrs[i].sh_type == elf::SHT_DYNSYM && shdrs[i].sh_entsize > 0) {
                return shdrs[i].sh_size / shdrs[i].sh_entsize;
            }
        }
        return 0;
    }

    void CollectExports32(const elf::Elf32_Sym* symtab, const char* strtab,
                          size_t symCount, uintptr_t bias) {
        for (size_t i = 1; i < symCount; i++) {
            const auto& sym = symtab[i];
            uint8_t bind = elf::ELF32_ST_BIND(sym.st_info);
            uint8_t type = elf::ELF32_ST_TYPE(sym.st_info);

            if ((bind == elf::STB_GLOBAL || bind == elf::STB_WEAK) &&
                (type == elf::STT_FUNC || type == elf::STT_OBJECT) &&
                sym.st_shndx != elf::SHN_UNDEF) {

                ExportEntry entry{};
                std::strncpy(entry.name, strtab + sym.st_name, sizeof(entry.name) - 1);
                entry.addr = reinterpret_cast<void*>(
                    static_cast<uintptr_t>(bias + sym.st_value));
                exports_.push_back(entry);
            }
        }
    }
};

} // namespace ksl
