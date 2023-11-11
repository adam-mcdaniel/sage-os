#include <elf.h>
#include <util.h>
#include <kmalloc.h>
#include <stdbool.h>

#define ELF_DEBUG
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
        if (header.e_ident[EI_OSABI] >= 64 && header.e_ident[EI_OSABI] <= 255) {
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

    debugf("\n");
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
