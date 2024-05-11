#include <compiler.h>
#include <debug.h>
#include <config.h>
#include <csr.h>
#include <gpu.h>
#include <kmalloc.h>
#include <list.h>
#include <lock.h>
#include <sbi.h>  // sbi_xxx()
#include <symbols.h>
#include <util.h>  // strcmp
#include <mmu.h>
#include <page.h>
#include <csr.h>
#include <trap.h>
#include <block.h>
#include <rng.h>
#include <vfs.h>
#include <stat.h>
#include <elf.h>
#include <process.h>
#include <sched.h>

// #define ELF_DEBUG
#ifdef ELF_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

bool elf_is_little_endian(Elf64_Ehdr header) {
    return header.e_ident[EI_DATA] == 1;
}

bool elf_is_big_endian(Elf64_Ehdr header) {
    return header.e_ident[EI_DATA] == 2;
}

char *elf_get_machine_name(Elf64_Ehdr header) {
    switch (header.e_machine) {
    case 0: return "No machine";
    case 1: return "AT&T WE 32100";
    case 2: return "SPARC";
    case 3: return "Intel 80386";
    case 4: return "Motorola 68000";
    case 5: return "Motorola 88000";
    case 6: return "(reserved for future use, was EM_486)";
    case 7: return "Intel 80860";
    case 8: return "MIPS I Architecture";
    case 9: return "IBM System/370 Processor";
    case 10: return "MIPS RS3000 Little-endian";
    case 11:
    case 12:
    case 13:
    case 14:
        return "(reserved for future use)";

    case 15: return "HPPA";
    case 16: return "(reserved for future use)";
    case 17: return "Fujitsu VPP500";
    case 18: return "Enhanced SPARC";
    case 19: return "Intel 80960";
    case 20: return "PowerPC";
    case 21: return "64-bit PowerPC";
    case 22: return "IBM System/390 Processor";
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
         return "(reserved for future use)";

    case 36: return "NEC V800";
    case 37: return "Fujitsu FR20";
    case 38: return "TRW RH-32";
    case 39: return "Motorola RCE";
    case 40: return "ARM 32-bit architecture (AARCH32)";
    case 41: return "Digital Alpha";
    case 42: return "Hitatchi SH";
    case 43: return "SPARC Version 9";
    case 44: return "Siemens TriCore embedded processor";
    case 45: return "Argonaut RISC Core, Argonaut Technologies Inc.";
    case 46: return "Hitachi H8/300";
    case 47: return "Hitachi H8/300H";
    case 48: return "Hitachi H8S";
    case 49: return "Hitachi H8/500";
    case 50: return "Intel IA-64 processor architecture";
    case 51: return "Stanford MIPS-X";
    case 52: return "Motorola Coldfire";
    case 53: return "Motorola M68HC12";
    case 54: return "Fujitsu MMA Multimedia Accelerator";
    case 55: return "Siemens PCP";
    case 56: return "Sony nCPU embedded RISC processor";
    case 57: return "Denso NDR1 microprocessor";
    case 58: return "Motorola Star*Core processor";
    case 59: return "Toyota ME16 processor";
    case 60: return "STMicroelectronics ST100 processor";
    case 61: return "Advanced Logic Corp. TinyJ embedded processor family";
// EM_68HC11	70	Motorola MC68HC11 Microcontroller
// EM_68HC08	71	Motorola MC68HC08 Microcontroller
// EM_68HC05	72	Motorola MC68HC05 Microcontroller
    case 62: return "AMD x86-64 architecture";
    case 63: return "Sony DSP Processor";
    case 64: return "Digital Equipment Corp. PDP-10";
    case 65: return "Digital Equipment Corp. PDP-11";
    case 66: return "Siemens FX66 microcontroller";
    case 67: return "STMicroelectronics ST9+ 8/16 bit microcontroller";
    case 68: return "STMicroelectronics ST7 8-bit microcontroller";
    case 69: return "Motorola MC68HC16 Microcontroller";
    case 70: return "Motorola MC68HC11 Microcontroller";
    case 71: return "Motorola MC68HC08 Microcontroller";
    case 72: return "Motorola MC68HC05 Microcontroller";
    case 73: return "Silicon Graphics SVx";
    case 74: return "STMicroelectronics ST19 8-bit microcontroller";
    case 75: return "Digital VAX";
    case 76: return "Axis Communications 32-bit embedded processor";
    case 77: return "Infineon Technologies 32-bit embedded processor";
    case 78: return "Element 14 64-bit DSP Processor";
    case 79: return "LSI Logic 16-bit DSP Processor";
    case 80: return "Donald Knuth's educational 64-bit processor";
    case 81: return "Harvard University machine-independent object files";
    case 82: return "SiTera Prism";
    case 83: return "Atmel AVR 8-bit microcontroller";
    case 84: return "Fujitsu FR30";
    case 85: return "Mitsubishi D10V";
    case 86: return "Mitsubishi D30V";
    case 87: return "NEC v850";
    case 88: return "Mitsubishi M32R";
    case 89: return "Matsushita MN10300";
// EM_MN10200	90	Matsushita MN10200
// EM_PJ	91	picoJava
// EM_OPENRISC	92	OpenRISC 32-bit embedded processor
// EM_ARC_A5	93	ARC Cores Tangent-A5
// EM_XTENSA	94	Tensilica Xtensa Architecture
// EM_VIDEOCORE	95	Alphamosaic VideoCore processor
// EM_TMM_GPP	96	Thompson Multimedia General Purpose Processor
// EM_NS32K	97	National Semiconductor 32000 series
// EM_TPC	98	Tenor Network TPC processor
// EM_SNP1K	99	Trebia SNP 1000 processor
// EM_ST200	100	
    case 90: return "Matsushita MN10200";
    case 91: return "picoJava";
    case 92: return "OpenRISC 32-bit embedded processor";
    case 93: return "ARC Cores Tangent-A5";
    case 94: return "Tensilica Xtensa Architecture";
    case 95: return "Alphamosaic VideoCore processor";
    case 96: return "Thompson Multimedia General Purpose Processor";
    case 97: return "National Semiconductor 32000 series";
    case 98: return "Tenor Network TPC processor";
    case 99: return "Trebia SNP 1000 processor";
    case 100: return "STMicroelectronics (www.st.com) ST200 microcontroller";
    default: 
        return NULL;
    }

}

char *elf_get_os_abi(Elf64_Ehdr header) {
    switch (header.e_ident[EI_OSABI]) {
    case 0:
        // System V
        return "System V";
    case 1:
        // HP-UX
        return "HP-UX";
    case 2:
        // NetBSD
        return "NetBSD";
    case 3:
        // Linux
        return "Linux";
    case 6:
        // Solaris
        return "Solaris";
    case 7:
        // AIX
        return "AIX";
    case 8:
        // IRIX
        return "IRIX";
    case 9:
        // FreeBSD
        return "FreeBSD";
    case 10:
        // Tru64
        return "Tru64";
    case 11:
        // Novell Modesto
        return "Novell Modesto";
    case 12:
        // OpenBSD
        return "OpenBSD";
    case 13:
        // OpenVMS
        return "OpenVMS";
    case 14:
        // NonStop Kernel
        return "HP NonStop Kernel";
    default:
        if (header.e_ident[EI_OSABI] >= 64) {
            return "Architecture-specific value";
        } else {
            return NULL;
        }
    }
}

bool elf_is_valid_header(Elf64_Ehdr header) {
    // Confirm magic number
    if (header.e_ident[EI_MAG0] != 0x7f ||
        header.e_ident[EI_MAG1] != 'E' ||
        header.e_ident[EI_MAG2] != 'L' ||
        header.e_ident[EI_MAG3] != 'F') {
        return false;
    }

    // Confirm class is valid
    switch (header.e_ident[EI_CLASS]) {
    case 1:
        // 32-bit
        break;
    case 2:
        // 64-bit
        break;
    default:
        // Invalid class
        // debugf("Invalid class (%u)\n", header.e_ident[EI_CLASS]);
        return false;
    }

    // Confirm data encoding is valid
    switch (header.e_ident[EI_DATA]) {
    case 1:
        // Little endian
        break;
    case 2:
        // Big endian
        break;
    default:
        // Invalid data encoding
        // debugf("Invalid data encoding (%u)\n", header.e_ident[EI_DATA]);
        return false;
    }

    // Confirm version is valid
    if (header.e_ident[EI_VERSION] != 1) { // EV_CURRENT
        // debugf("Invalid version (0=EV_NONE)\n");
        return false;
    }

    // Check the OS ABI
    char *os_abi = elf_get_os_abi(header);
    if (!os_abi) {
        // debugf("Invalid OS ABI\n");
        return false;
    }
    return true;
}

bool elf_is_valid_program_header(Elf64_Phdr header) {
    switch (header.p_type) {
    case PT_NULL:
        // Unused entry
        break;
    case PT_LOAD:
        // Loadable segment
        // Confirm that the memsize is not zero
        if (header.p_memsz == 0) {
            // debugf("Invalid memsize\n");
            return false;
        }

        // Confirm that the alignment is a power of 2
        if ((header.p_align & (header.p_align - 1)) != 0) {
            // debugf("Invalid alignment\n");
            return false;
        }

        // Confirm that the alignment is not zero
        if (header.p_align == 0) {
            // debugf("Invalid alignment\n");
            return false;
        }
        break;
    case PT_DYNAMIC:
        // Dynamic linking information
        break;
    case PT_INTERP:
        // Interpreter information
        break;
    case PT_NOTE:
        // Auxiliary information
        break;
    case PT_SHLIB:
        // Reserved
        break;
    case PT_PHDR:
        // Segment containing program header table itself
        break;
    case PT_TLS:
        // Thread-local storage segment
        break;
    case PT_LOOS:
        // Start of OS-specific
        break;
    case PT_HIOS:
        // End of OS-specific
        break;
    case PT_LOPROC:
        // Start of processor-specific
        break;
    case PT_HIPROC:
        // End of processor-specific
        break;

    default:
        if (header.p_type >= PT_LOOS && header.p_type <= PT_HIOS) {
            // debugf("OS-specific\n");
        } else if (header.p_type >= PT_LOPROC && header.p_type <= PT_HIPROC) {
            // debugf("Processor-specific\n");
        } else {
            // debugf("(unrecognized/invalid)\n");
            return false;
        }
        break;
    }
    return true;
}

void elf_debug_header(Elf64_Ehdr header) {
    if (!elf_is_valid_header(header)) {
        debugf("Invalid ELF header\n");
        return;
    }
    debugf("ELF Header:\n");
    debugf("   Magic:   ");
    for (int i = 0; i < EI_NIDENT; i++) {
        debugf("%02x ", header.e_ident[i]);
    }
    debugf("(valid)\n");
    debugf("   Class:                              ");
    switch (header.e_ident[EI_CLASS]) {
    case 0:
        debugf("(invalid)\n");
        break;
    case 1:
        debugf("ELF 32-bit\n");
        break;
    case 2:
        debugf("ELF 64-bit\n");
        break;
    default:
        debugf("(unrecognized/invalid)\n");
        break;
    }
    debugf("   Data:                               ");
    switch (header.e_ident[EI_DATA]) {
    case 1:
        debugf("(little endian)\n");
        break;
    case 2:
        debugf("(big endian)\n");
        break;
    default:
        debugf("(invalid)\n");
        break;
    }
    debugf("   Version:                            ");
    switch (header.e_ident[EI_VERSION]) {
    case 0:
        debugf("(invalid)\n");
        break;
    case 1:
        debugf("(current)\n");
        break;
    default:
        debugf("(unrecognized/invalid)\n");
        break;
    }
    debugf("   OS/ABI:                             ");
    char *os_abi = elf_get_os_abi(header);
    if (os_abi) {
        debugf("(%s)\n", os_abi);
    } else {
        debugf("(invalid)\n");
    }
    debugf("   ABI Version:                        %u\n", header.e_ident[EI_ABIVERSION]);
    debugf("   Type:                               ");
    switch (header.e_type) {
    case ET_NONE:
        debugf("None\n");
        break;
    case ET_REL:
        debugf("Relocatable file\n");
        break;
    case ET_EXEC:
        debugf("Executable file\n");
        break;
    case ET_DYN:
        debugf("Shared object file\n");
        break;
    case ET_CORE:
        debugf("Core file\n");
        break;
    case ET_LOOS:
        debugf("Operating system-specific (LOOS)\n");
        break;
    case ET_HIOS:
        debugf("Operating system-specific (HIOS)\n");
        break;
    case ET_LOPROC:
        debugf("Processor-specific (LOPROC)\n");
        break;
    case ET_HIPROC:
        debugf("Processor-specific (HIPROC)\n");
        break;
    default:
        debugf("(unrecognized/invalid)\n");
        break;
    }

    debugf("   Machine:                            ");
    char *machine_name = elf_get_machine_name(header);
    if (machine_name) {
        debugf("%s\n", machine_name);
    } else {
        debugf("(unrecognized/invalid)\n");
    }

    debugf("   Version:                            %u\n", header.e_version);
    debugf("   Entry point address:                0x%lx\n", header.e_entry);
    debugf("   Start of program headers:           0x%lx\n", header.e_phoff);
    debugf("   Start of section headers:           0x%lx\n", header.e_shoff);
    debugf("   Flags:                              0x%x\n", header.e_flags);
    debugf("   Size of this header:                %u\n", header.e_ehsize);
    debugf("   Size of program headers:            %u\n", header.e_phentsize);
    debugf("   Number of program headers:          %u\n", header.e_phnum);
    debugf("   Size of section headers:            %u\n", header.e_shentsize);
    debugf("   Number of section headers:          %u\n", header.e_shnum);
    debugf("   Section header string table index:  %u\n", header.e_shstrndx);
}

void elf_debug_program_header(Elf64_Phdr header) {
    debugf("Program Header:\n");
    // debugf("   Type:             0x%x\n", header.p_type);
    debugf("   Type:             ");
    switch (header.p_type) {
    case PT_NULL:
        debugf("Unused entry\n");
        break;
    case PT_LOAD:
        debugf("Loadable segment\n");
        break;
    case PT_DYNAMIC:
        debugf("Dynamic linking information\n");
        break;
    case PT_INTERP:
        debugf("Interpreter information\n");
        break;
    case PT_NOTE:
        debugf("Auxiliary information\n");
        break;
    case PT_SHLIB:
        debugf("Reserved\n");
        break;
    case PT_PHDR:
        debugf("Segment containing program header table itself\n");
        break;
    case PT_TLS:
        debugf("Thread-local storage segment\n");
        break;
    case PT_LOOS:
        debugf("Start of OS-specific\n");
        break;
    case PT_HIOS:
        debugf("End of OS-specific\n");
        break;
    case PT_LOPROC:
        debugf("Start of processor-specific\n");
        break;
    case PT_HIPROC:
        debugf("End of processor-specific\n");
        break;
    default:
        if (header.p_type >= PT_LOOS && header.p_type <= PT_HIOS) {
            debugf("OS-specific\n");
        } else if (header.p_type >= PT_LOPROC && header.p_type <= PT_HIPROC) {
            debugf("Processor-specific\n");
        } else {
            debugf("(unrecognized/invalid)\n");
        }
        break;
    }

    debugf("   Flags:            ");
    if (header.p_flags & PF_R) {
        debugf("R");
    }
    if (header.p_flags & PF_W) {
        debugf("W");
    }
    if (header.p_flags & PF_X) {
        debugf("X");
    }
    if (header.p_flags & ~(PF_R | PF_W | PF_X)) {
        debugf("(unrecognized/invalid)");
    }
    debugf(" ( ");
    if (header.p_flags & PF_R) {
        debugf("Read ");
    }
    if (header.p_flags & PF_W) {
        debugf("Write ");
    }
    if (header.p_flags & PF_X) {
        debugf("Exec ");
    }
    debugf(")");
    

    debugf("\n");
    debugf("   Offset:           0x%lx\n", header.p_offset);
    debugf("   Virtual Address:  0x%lx\n", header.p_vaddr);
    debugf("   Physical Address: 0x%lx\n", header.p_paddr);
    debugf("   File Size:        0x%lx\n", header.p_filesz);
    debugf("   Memory Size:      0x%lx\n", header.p_memsz);
    debugf("   Alignment:        0x%lx\n", header.p_align);
}


Elf64_Phdr elf_get_text_section(Elf64_Ehdr elf_header, Elf64_Phdr *program_headers) {
    // Find the `text` header
    for (uint32_t i = 0; i < elf_header.e_phnum; i++) {
        if (program_headers[i].p_type == PT_LOAD && program_headers[i].p_flags & PF_X) {
            return program_headers[i];
        }
    }
}

Elf64_Phdr elf_get_bss_section(Elf64_Ehdr elf_header, Elf64_Phdr *program_headers) {
    // Find the `bss` header
    for (uint32_t i = 0; i < elf_header.e_phnum; i++) {
        if (program_headers[i].p_type == PT_LOAD && program_headers[i].p_flags & PF_W) {
            return program_headers[i];
        }
    }
}

Elf64_Phdr elf_get_data_section(Elf64_Ehdr elf_header, Elf64_Phdr *program_headers) {
    // Find the `data` header
    for (uint32_t i = 0; i < elf_header.e_phnum; i++) {
        if (program_headers[i].p_type == PT_LOAD && program_headers[i].p_flags & PF_W && !(program_headers[i].p_flags & PF_X)) {
            return program_headers[i];
        }
    }
}

Elf64_Phdr elf_get_rodata_section(Elf64_Ehdr elf_header, Elf64_Phdr *program_headers) {
    // Find the `rodata` header
    for (uint32_t i = 0; i < elf_header.e_phnum; i++) {
        if (program_headers[i].p_type == PT_LOAD && !(program_headers[i].p_flags & PF_W) && !(program_headers[i].p_flags & PF_X)) {
            return program_headers[i];
        }
    }
}

Elf64_Phdr elf_get_dynamic_section(Elf64_Ehdr elf_header, Elf64_Phdr *program_headers) {
    // Find the `dynamic` header
    for (uint32_t i = 0; i < elf_header.e_phnum; i++) {
        if (program_headers[i].p_type == PT_DYNAMIC) {
            return program_headers[i];
        }
    }
}

bool elf_is_valid_rodata(Elf64_Phdr rodata) {
    return rodata.p_type == PT_LOAD && !(rodata.p_flags & PF_W) && !(rodata.p_flags & PF_X);
}

bool elf_is_valid_bss(Elf64_Phdr bss) {
    return bss.p_type == PT_LOAD && bss.p_flags & PF_W && !(bss.p_flags & PF_X);
}

bool elf_is_valid_data(Elf64_Phdr data) {
    return data.p_type == PT_LOAD && data.p_flags & PF_W && !(data.p_flags & PF_X);
}

bool elf_is_valid_text(Elf64_Phdr text) {
    return text.p_type == PT_LOAD && text.p_flags & PF_X;
}

Elf64_Shdr elf_get_string_table_header(Elf64_Ehdr header, Elf64_Shdr *section_headers, uint8_t *elf) {
    // Find the string table
    Elf64_Shdr strtab_header = {0};
    for (uint32_t i = 0; i < header.e_shnum; i++) {
        if (section_headers[i].sh_type == SHT_STRTAB) {
            strtab_header = section_headers[i];
            break;
        }
    }

    if (strtab_header.sh_size == 0) {
        debugf("No string table\n");
        return strtab_header;
    }

    return strtab_header;
}

char *elf_get_string_table(Elf64_Ehdr header, Elf64_Shdr *section_headers, uint8_t *elf) {
    // Find the string table
    Elf64_Shdr strtab_header = elf_get_string_table_header(header, section_headers, elf);
    if (strtab_header.sh_size == 0) {
        debugf("No string table\n");
        return NULL;
    }

    // Get the string table
    char *strtab = (char*)(elf + strtab_header.sh_offset);
    return strtab;
}

Elf64_Shdr elf_get_symbol_table_header(Elf64_Ehdr header, Elf64_Shdr *section_headers, uint8_t *elf) {
    // Find the symbol table
    Elf64_Shdr symtab_header = {0};
    for (uint32_t i = 0; i < header.e_shnum; i++) {
        if (section_headers[i].sh_type == SHT_SYMTAB) {
            symtab_header = section_headers[i];
            break;
        }
    }

    if (symtab_header.sh_size == 0) {
        debugf("No symbols\n");
        return symtab_header;
    }

    return symtab_header;
}

Elf64_Sym *elf_get_symbol_table(Elf64_Ehdr header, Elf64_Shdr *section_headers, uint8_t *elf) {
    // Find the symbol table
    Elf64_Shdr symtab_header = elf_get_symbol_table_header(header, section_headers, elf);
    if (symtab_header.sh_size == 0) {
        debugf("No symbols\n");
        return NULL;
    }

    // Get the symbol table
    Elf64_Sym *symtab = (Elf64_Sym*)(elf + symtab_header.sh_offset);
    return symtab;
}

Elf64_Sym *elf_get_symbol(Elf64_Ehdr header, Elf64_Shdr *section_headers, uint8_t *elf, char *name) {
    // Get the symbol table
    Elf64_Shdr symtab_header = elf_get_symbol_table_header(header, section_headers, elf);
    Elf64_Sym *symtab = elf_get_symbol_table(header, section_headers, elf);
    if (symtab == NULL) {
        debugf("No symbols\n");
        return NULL;
    }
    uint32_t num_symbols = symtab_header.sh_size / symtab_header.sh_entsize;

    // Get the string table
    char *strtab = elf_get_string_table(header, section_headers, elf);
    if (symtab_header.sh_size == 0) {
        debugf("No symbols\n");
        return NULL;
    }

    // Find the symbol
    for (uint32_t i = 0; i < num_symbols; i++) {
        if (strcmp(strtab + symtab[i].st_name, name) == 0) {
            return &symtab[i];
        }
    }
    return NULL;
}

void elf_print_symbols(Elf64_Ehdr header, Elf64_Shdr *section_headers, uint8_t *elf) {
    // Get the symbol table
    Elf64_Shdr symtab_header = elf_get_symbol_table_header(header, section_headers, elf);
    Elf64_Sym *symtab = elf_get_symbol_table(header, section_headers, elf);
    if (symtab == NULL) {
        debugf("No symbols\n");
        return;
    }
    uint32_t num_symbols = symtab_header.sh_size / symtab_header.sh_entsize;

    // Get the string table
    Elf64_Shdr strtab_header = elf_get_string_table_header(header, section_headers, elf);
    char *strtab = (char*)(elf + strtab_header.sh_offset);

    if (symtab_header.sh_size == 0) {
        debugf("No symbols\n");
        return;
    }

    // Print the symbols
    debugf("Symbols:\n");
    for (uint32_t i = 0; i < num_symbols; i++) {
        debugf("   %s\n", strtab + symtab[i].st_name);
    }
}

bool elf_is_valid(const uint8_t *elf) {
    // Read the ELF header
    Elf64_Ehdr header;
    memcpy(&header, elf, sizeof(header));
    if (!elf_is_valid_header(header)) {
        debugf("Invalid ELF header\n");
        return false;
    }
    elf_debug_header(header);
    
    // Read the program headers
    Elf64_Phdr *program_headers = kmalloc(header.e_phentsize * header.e_phnum);
    memcpy(program_headers, elf + header.e_phoff, header.e_phentsize * header.e_phnum);
    for (uint32_t i = 0; i < header.e_phnum; i++) {
        if (!elf_is_valid_program_header(program_headers[i])) {
            debugf("Invalid program header #%u\n", i);
            // return false;
        }
    }
    kfree(program_headers);
    return true;
}

int elf_create_process(Process *p, const uint8_t *elf) {
    if (!elf_is_valid_header(*(Elf64_Ehdr*)elf)) {
        debugf("Invalid ELF header\n");
        return 1;
    }

    if (p == NULL) {
        debugf("Process is NULL\n");
        return 1;
    }

    // Check if the process has a page table
    if (p->rcb.ptable == NULL) {
        debugf("Process does not have a page table\n");
        // Create a page table
        p->rcb.ptable = mmu_table_create();
        if (p->rcb.ptable == NULL) {
            debugf("Failed to create page table\n");
            return 1;
        }
    }

    // Read the ELF header
    Elf64_Ehdr header;
    memcpy(&header, elf, sizeof(header));
    if (!elf_is_valid_header(header)) {
        debugf("Invalid ELF header\n");
        return 1;
    }
    elf_debug_header(header);
    
    // Read the program headers
    Elf64_Phdr *program_headers = kmalloc(header.e_phentsize * header.e_phnum);
    memcpy(program_headers, elf + header.e_phoff, header.e_phentsize * header.e_phnum);
    for (uint32_t i = 0; i < header.e_phnum; i++) {
        if (!elf_is_valid_program_header(program_headers[i])) {
            debugf("Invalid program header #%u\n", i);
        }

        elf_debug_program_header(program_headers[i]);
    }

    // Go through the program headers and find the text, bss, rodata, srodata, and data segments
    uint64_t permission_bits = PB_READ | PB_EXECUTE | PB_WRITE | PB_USER;

    // Get sum of the sizes of all the segments
    Elf64_Phdr text_header = elf_get_text_section(header, program_headers);
    Elf64_Phdr rodata_header = elf_get_rodata_section(header, program_headers);
    Elf64_Phdr bss_header = elf_get_bss_section(header, program_headers);
    Elf64_Phdr data_header = elf_get_data_section(header, program_headers);
    debugf("Text header:\n");
    elf_debug_program_header(text_header);
    debugf("RODATA header:\n");
    elf_debug_program_header(rodata_header);
    debugf("BSS header:\n");
    elf_debug_program_header(bss_header);
    debugf("DATA header:\n");
    elf_debug_program_header(data_header);

    // Get the sizes of the segments
    debugf("Getting sizes of segments\n");
    #define min(a, b) ((a) < (b)? (a) : (b))
    #define max(a, b) ((a) > (b)? (a) : (b))
    uint64_t text_size = ALIGN_UP_TO_PAGE(elf_is_valid_text(text_header)? text_header.p_memsz : 0) + 0x1000;
    debugf("Text size: %x\n", text_size);
    uint64_t rodata_size = ALIGN_UP_TO_PAGE(elf_is_valid_rodata(rodata_header)? rodata_header.p_memsz : 0) + 0x1000;
    debugf("RODATA size: %x\n", rodata_size);
    uint64_t bss_size = ALIGN_UP_TO_PAGE(elf_is_valid_bss(bss_header)? bss_header.p_memsz : 0) + 0x1000;
    debugf("BSS size: %x\n", bss_size);
    uint64_t data_size = ALIGN_UP_TO_PAGE(elf_is_valid_data(data_header) && data_header.p_vaddr != bss_header.p_vaddr? data_header.p_memsz : 0) + 0x1000;
    debugf("DATA size: %x\n", data_size);

    // p->heap_size = USER_HEAP_SIZE * 2;
    // p->stack_size = USER_STACK_SIZE * 2;
    uint64_t total_size = text_size + rodata_size + bss_size + data_size + 0x10000;
    debugf("Total size: %x\n", total_size);
    // Allocate the memory for the segments
    uint8_t *segments = (uint8_t*)page_znalloc(ALIGN_UP_TO_PAGE(total_size) / PAGE_SIZE_4K);
    // memset(segments, 0, total_size);
    p->image = segments;
    p->image_size = total_size;

    // Get the pointers to the segments
    uint8_t *text = segments;
    uint8_t *rodata = text + text_size + 0x1000;
    uint8_t *bss = rodata + rodata_size + 0x1000;
    uint8_t *data = bss + bss_size + 0x1000;
    // p->heap = data + data_size;
    // p->stack = p->heap + p->heap_size;
    if (!text_size) text = NULL;
    if (!rodata_size) rodata = NULL;
    if (!bss_size) bss = NULL;
    if (!data_size || data_header.p_vaddr == bss_header.p_vaddr) {
        data = NULL;
        data_size = 0;
    }

    // Copy the segments into the allocated memory
    if (text) {
        debugf("Copying text segment\n");
        memcpy(text, elf + text_header.p_offset, text_header.p_filesz);
    }
    if (rodata) {
        debugf("Copying rodata segment\n");
        memcpy(rodata, elf + rodata_header.p_offset, rodata_header.p_filesz);
    }
    if (data) {
        debugf("Copying data segment\n");
        memcpy(data, elf + data_header.p_offset, data_header.p_filesz);
    }

    // void process_asm_run(void);
    // unsigned long trans_trampoline_start = mmu_translate(kernel_mmu_table, trampoline_thread_start);
    // unsigned long trans_trampoline_trap  = mmu_translate(kernel_mmu_table, trampoline_trap_start);
    // unsigned long trans_process_asm_run  = mmu_translate(kernel_mmu_table, process_asm_run);
    // unsigned long trans_os_trap_handler  = mmu_translate(kernel_mmu_table, os_trap_handler);
    // mmu_map(p->rcb.ptable, trampoline_thread_start, trans_trampoline_start, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER);
    // mmu_map(p->rcb.ptable, trampoline_trap_start, trans_trampoline_trap, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER);
    // // Map the trap stack
    // mmu_map(p->rcb.ptable, process_asm_run, trans_process_asm_run, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER);
    // mmu_map(p->rcb.ptable, os_trap_handler, trans_os_trap_handler, MMU_LEVEL_4K,
    //         PB_READ | PB_EXECUTE | PB_USER);
    // mmu_map(p->rcb.ptable, trans_frame, trans_frame, MMU_LEVEL_4K, PB_READ | PB_WRITE | PB_EXECUTE | PB_USER);
    // mmu_map(p->rcb.ptable, p->frame.stvec, p->frame.stvec, MMU_LEVEL_4K, PB_READ | PB_WRITE | PB_EXECUTE | PB_USER);

    // Create the process
    p->text = text;
    p->text_vaddr = text? (uint8_t *)text_header.p_vaddr : NULL;
    p->text_size = text_size;
    debugf("Text: %p\n", p->text);
    debugf("Text vaddr: %p\n", p->text_vaddr);
    debugf("Text size: %lu\n", p->text_size);
    
    p->bss = bss;
    p->bss_vaddr = bss? (uint8_t *)bss_header.p_vaddr : NULL;
    p->bss_size = bss_size;
    debugf("BSS: %p\n", p->bss);
    debugf("BSS vaddr: %p\n", p->bss_vaddr);
    debugf("BSS size: %lu\n", p->bss_size);
    

    p->rodata = rodata;
    p->rodata_vaddr = rodata? (uint8_t *)rodata_header.p_vaddr : NULL;
    p->rodata_size = rodata_size;
    debugf("RODATA: %p\n", p->rodata);
    debugf("RODATA vaddr: %p\n", p->rodata_vaddr);
    debugf("RODATA size: %lu\n", p->rodata_size);

    p->data = data;
    p->data_vaddr = data? (uint8_t *)data_header.p_vaddr : NULL;
    p->data_size = data_size;
    debugf("DATA: %p\n", p->data);
    debugf("DATA vaddr: %p\n", p->data_vaddr);
    debugf("DATA size: %lu\n", p->data_size);

    // Store all the pages in the `segments` array
    for (uint64_t i = 0; i < total_size / PAGE_SIZE; i++) {
        list_add_ptr(p->rcb.image_pages, segments + i * PAGE_SIZE);
    }

    // // Allocate stack and heap
    // if (!p->rcb.heap_pages) {
    //     p->rcb.heap_pages = list_new();
    //     memset(p->heap, 0, p->heap_size);
    //     for (uint64_t i = 0; i < p->heap_size / PAGE_SIZE; i++)0x1000 {
    //         list_add_ptr(p->rcb.heap_pages, p->heap + i * PAGE_SIZE);
    //         rcb_map(&p->rcb, 
    //                 USER_HEAP_BOTTOM + i * PAGE_SIZE, 
    //                 kernel_mmu_translate((uint64_t)p->heap + i * PAGE_SIZE), 
    //                 PAGE_SIZE,
    //                 permission_bits);
    //     }
    // }
    

    // if (!p->rcb.stack_pages) {
    //     p->rcb.stack_pages = list_new();
    //     memset(p->stack, 0, p->stack_size);
    //     for (uint64_t i = 0; i < p->stack_size / PAGE_SIZE; i++) {
    //         list_add_ptr(p->rcb.stack_pages, p->stack + i * PAGE_SIZE);
    //         rcb_map(&p->rcb, 
    //                 USER_STACK_BOTTOM + i * PAGE_SIZE, 
    //                 kernel_mmu_translate((uint64_t)p->stack + i * PAGE_SIZE), 
    //                 PAGE_SIZE,
    //                 permission_bits);
    //     }
    // }

    // Create the environment
    // if (!p->rcb.environemnt) {
    //     p->rcb.environemnt = map_new();
    // }

    // // Create the file descriptors
    // if (!p->rcb.file_descriptors) {
    //     p->rcb.file_descriptors = list_new();
    // }

    // Set sepc of the process's trap frame

    p->frame->sepc = header.e_entry;

    // mmu_translate(p->rcb.ptable, p->frame.stvec);

    // CSR_READ(p->frame.sie, "sie");

    // Map all the segments into the page table
    // mmu_map_range(p->rcb.ptable, 
    //             segments,
    //             segments + total_size,
    //             (uint64_t)segments,
    //             MMU_LEVEL_4K,
    //             permission_bits);
    // rcb_map(&p->rcb, 
    //             segments,
    //             kernel_mmu_translate((uint64_t)segments), 
    //             total_size,
    //             permission_bits);

    // // Map the segments into the page table
    if (text) {
        debugf("Mapping text segment\n");
        rcb_map(&p->rcb, 
                text_header.p_vaddr, 
                kernel_mmu_translate((uint64_t)text), 
                (text_size / 0x1000 + 1) * 0x1000,
                permission_bits);
    }
    if (rodata) {
        debugf("Mapping rodata segment\n");
        // mmu_map_range(p->rcb.ptable, 
        //             rodata_header.p_vaddr, 
        //             rodata_header.p_vaddr + rodata_size, 
        //             (uint64_t)rodata, 
        //             MMU_LEVEL_4K,
        //             permission_bits);
        rcb_map(&p->rcb, 
                rodata_header.p_vaddr, 
                kernel_mmu_translate((uint64_t)rodata), 
                (rodata_size / 0x1000) * 0x1000,
                permission_bits);
    }
    if (bss) {
        debugf("Mapping bss segment\n");
        // mmu_map_range(p->rcb.ptable, 
        //             bss_header.p_vaddr, 
        //             bss_header.p_vaddr + bss_size, 
        //             (uint64_t)bss, 
        //             MMU_LEVEL_4K,
        //             permission_bits);
        rcb_map(&p->rcb, 
                bss_header.p_vaddr, 
                kernel_mmu_translate((uint64_t)bss), 
                (bss_size / 0x1000) * 0x1000,
                permission_bits);
    }
    if (data) {
        debugf("Mapping data segment\n");
        // mmu_map_range(p->rcb.ptable, 
        //             data_head & ~0xfffer.p_vaddr, 
        //             data_header.p_vaddr + data_size, 
        //             (uint64_t)data, 
        //             MMU_LEVEL_4K,
        //             permission_bits);
        rcb_map(&p->rcb, 
                data_header.p_vaddr, 
                kernel_mmu_translate((uint64_t)data), 
                (data_size / 0x1000) * 0x1000,
                permission_bits);
    }

    Elf64_Shdr *section_headers = kmalloc(header.e_shentsize * header.e_shnum);
    memcpy(section_headers, elf + header.e_shoff, header.e_shentsize * header.e_shnum);
    elf_print_symbols(header, section_headers, (uint8_t*)elf);

    // Copy the data into the process's memory
    if (text) {
        debugf("Copying text segment from %p to %p\n", elf + text_header.p_offset, text);
        memcpy(text, elf + text_header.p_offset, text_size);
    }
    if (rodata) {
        debugf("Copying rodata segment\n");
        memcpy(rodata, elf + rodata_header.p_offset, rodata_size);
    }
    if (data) {
        debugf("Copying data segment\n");
        memcpy(data, elf + data_header.p_offset, data_size);
    }
    if (bss) {
        debugf("Copying data segment\n");
        // memset(bss, 0, bss_size);
        memcpy(bss, elf + bss_header.p_offset, data_size);
    }
    
    // Get _bss_start and _bss_end
    Elf64_Sym *bss_start_sym = elf_get_symbol(header, section_headers, elf, "_bss_start");
    Elf64_Sym *bss_end_sym = elf_get_symbol(header, section_headers, elf, "_bss_end");

    if (bss_start_sym && bss_end_sym) {
        // Clear the bss segment
        debugf("Found _bss_start=%p and _bss_end=%p symbols\n", bss_start_sym->st_value, bss_end_sym->st_value);
        uint8_t *bss_start = mmu_translate(p->rcb.ptable, bss_start_sym->st_value);
        uint8_t *bss_end = mmu_translate(p->rcb.ptable, bss_end_sym->st_value);
        debugf("Clearing bss segment from %p to %p\n", bss_start, bss_end);
        // memset(bss_start, 0, bss_end - bss_start);
    } else {
        debugf("Could not find _bss_start and _bss_end symbols\n");
    }


    
    trap_frame_set_stack_pointer(p->frame, USER_STACK_TOP);
    trap_frame_set_heap_pointer(p->frame, USER_HEAP_BOTTOM);
    // Section headers
    // int64_t xregs[32];
    // double fregs[32];
    // uint64_t sepc;
    // uint64_t sstatus;
    // uint64_t sie;
    // uint64_t satp;
    // uint64_t sscratch;
    // uint64_t stvec;
    // uint64_t trap_satp;
    // uint64_t trap_stack;
    process_debug(p);
    return 0;
}