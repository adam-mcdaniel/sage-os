#include <stdbool.h>
#include <stdint.h>

#define NULL (void*)0

/* Legal values for p_type (segment type).  */
#define PT_NULL     0       /* Program header table entry unused */
#define PT_LOAD     1       /* Loadable program segment */
#define PT_DYNAMIC  2       /* Dynamic linking information */
#define PT_INTERP   3       /* Program interpreter */
#define PT_NOTE     4       /* Auxiliary information */
#define PT_SHLIB    5       /* Reserved */
#define PT_PHDR     6       /* Entry for header table itself */
#define PT_TLS      7       /* Thread-local storage segment */
#define PT_NUM      8       /* Number of defined types */
#define PT_LOOS     0x60000000  /* Start of OS-specific */
#define PT_GNU_EH_FRAME 0x6474e550  /* GCC .eh_frame_hdr segment */
#define PT_GNU_STACK    0x6474e551  /* Indicates stack executability */
#define PT_GNU_RELRO    0x6474e552  /* Read-only after relocation */
#define PT_LOSUNW   0x6ffffffa
#define PT_HISUNW   0x6fffffff
#define PT_HIOS     0x6fffffff  /* End of OS-specific */
#define PT_LOPROC   0x70000000  /* Start of processor-specific */
#define PT_HIPROC   0x7fffffff  /* End of processor-specific */

#define ET_NONE     0       /* No file type */
#define ET_REL      1       /* Relocatable file */
#define ET_EXEC     2       /* Executable file */
#define ET_DYN      3       /* Shared object file */
#define ET_CORE     4       /* Core file */
#define ET_LOOS     0xfe00  /* Operating system-specific */
#define ET_HIOS     0xfeff  /* Operating system-specific */
#define ET_LOPROC   0xff00  /* Processor-specific */
#define ET_HIPROC   0xffff  /* Processor-specific */

/* Legal values for p_flags (segment flags).  */
#define PF_X        (1 << 0)    /* Segment is executable */
#define PF_W        (1 << 1)    /* Segment is writable */
#define PF_R        (1 << 2)    /* Segment is readable */
#define PF_MASKOS   0x0ff00000  /* OS-specific */
#define PF_MASKPROC 0xf0000000  /* Processor-specific */

/* Legal values for sh_type (section type).  */
#define SHT_NULL          0     /* Section header table entry unused */
#define SHT_PROGBITS      1     /* Program data */
#define SHT_SYMTAB        2     /* Symbol table */
#define SHT_STRTAB        3     /* String table */
#define SHT_RELA          4     /* Relocation entries with addends */
#define SHT_HASH          5     /* Symbol hash table */
#define SHT_DYNAMIC       6     /* Dynamic linking information */
#define SHT_NOTE          7     /* Notes */
#define SHT_NOBITS        8     /* Program space with no data (bss) */
#define SHT_REL           9     /* Relocation entries, no addends */
#define SHT_SHLIB         10    /* Reserved */
#define SHT_DYNSYM        11    /* Dynamic linker symbol table */
#define SHT_INIT_ARRAY    14    /* Array of constructors */
#define SHT_FINI_ARRAY    15    /* Array of destructors */
#define SHT_PREINIT_ARRAY 16    /* Array of pre-constructors */
#define SHT_GROUP         17    /* Section group */
#define SHT_SYMTAB_SHNDX  18    /* Extended section indeces */
#define SHT_NUM           19    /* Number of defined types.  */
#define SHT_LOOS          0x60000000    /* Start OS-specific.  */
#define SHT_GNU_ATTRIBUTES 0x6ffffff5   /* Object attributes.  */
#define SHT_GNU_HASH      0x6ffffff6    /* GNU-style hash table.  */
#define SHT_GNU_LIBLIST   0x6ffffff7    /* Prelink library list */
#define SHT_CHECKSUM      0x6ffffff8    /* Checksum for DSO content.  */
#define SHT_LOSUNW        0x6ffffffa    /* Sun-specific low bound.  */


/* Legal values for sh_flags (section flags).  */
#define SHF_WRITE        (1 << 0)   /* Writable */
#define SHF_ALLOC        (1 << 1)   /* Occupies memory during execution */
#define SHF_EXECINSTR    (1 << 2)   /* Executable */
#define SHF_MERGE        (1 << 4)   /* Might be merged */
#define SHF_STRINGS      (1 << 5)   /* Contains nul-terminated strings */
#define SHF_INFO_LINK    (1 << 6)   /* `sh_info' contains SHT index */
#define SHF_LINK_ORDER   (1 << 7)   /* Preserve order after combining */
#define SHF_OS_NONCONFORMING (1 << 8)   /* Non-standard OS specific handling required */
#define SHF_GROUP        (1 << 9)   /* Section is member of a group.  */
#define SHF_TLS          (1 << 10)  /* Section hold thread-local data.  */
#define SHF_COMPRESSED   (1 << 11)  /* Section with compressed data. */
#define SHF_MASKOS       0x0ff00000 /* OS-specific.  */
#define SHF_MASKPROC     0xf0000000 /* Processor-specific */
/* RISC-V ELF Flags */
#define EF_RISCV_RVC                0x0001
#define EF_RISCV_FLOAT_ABI          0x0006
#define EF_RISCV_FLOAT_ABI_SOFT     0x0000
#define EF_RISCV_FLOAT_ABI_SINGLE   0x0002
#define EF_RISCV_FLOAT_ABI_DOUBLE   0x0004
#define EF_RISCV_FLOAT_ABI_QUAD     0x0006

/* RISC-V relocations.  */
#define R_RISCV_NONE             0
#define R_RISCV_32               1
#define R_RISCV_64               2
#define R_RISCV_RELATIVE         3
#define R_RISCV_COPY             4
#define R_RISCV_JUMP_SLOT        5
#define R_RISCV_TLS_DTPMOD32     6
#define R_RISCV_TLS_DTPMOD64     7
#define R_RISCV_TLS_DTPREL32     8
#define R_RISCV_TLS_DTPREL64     9
#define R_RISCV_TLS_TPREL32     10
#define R_RISCV_TLS_TPREL64     11
#define R_RISCV_BRANCH          16
#define R_RISCV_JAL             17
#define R_RISCV_CALL            18
#define R_RISCV_CALL_PLT        19
#define R_RISCV_GOT_HI20        20
#define R_RISCV_TLS_GOT_HI20    21
#define R_RISCV_TLS_GD_HI20     22
#define R_RISCV_PCREL_HI20      23
#define R_RISCV_PCREL_LO12_I    24
#define R_RISCV_PCREL_LO12_S    25
#define R_RISCV_HI20            26
#define R_RISCV_LO12_I          27
#define R_RISCV_LO12_S          28
#define R_RISCV_TPREL_HI20      29
#define R_RISCV_TPREL_LO12_I    30
#define R_RISCV_TPREL_LO12_S    31
#define R_RISCV_TPREL_ADD       32
#define R_RISCV_ADD8            33
#define R_RISCV_ADD16           34
#define R_RISCV_ADD32           35
#define R_RISCV_ADD64           36
#define R_RISCV_SUB8            37
#define R_RISCV_SUB16           38
#define R_RISCV_SUB32           39
#define R_RISCV_SUB64           40
#define R_RISCV_GNU_VTINHERIT   41
#define R_RISCV_GNU_VTENTRY     42
#define R_RISCV_ALIGN           43
#define R_RISCV_RVC_BRANCH      44
#define R_RISCV_RVC_JUMP        45
#define R_RISCV_RVC_LUI         46
#define R_RISCV_GPREL_I         47
#define R_RISCV_GPREL_S         48
#define R_RISCV_TPREL_I         49
#define R_RISCV_TPREL_S         50
#define R_RISCV_RELAX           51
#define R_RISCV_SUB6            52
#define R_RISCV_SET6            53
#define R_RISCV_SET8            54
#define R_RISCV_SET16           55
#define R_RISCV_SET32           56
#define R_RISCV_32_PCREL        57

// Define the elf word type
typedef uint64_t	Elf64_Addr;
typedef uint16_t	Elf64_Half;
typedef uint64_t	Elf64_Off;
typedef int32_t		Elf64_Sword;
typedef int64_t		Elf64_Sxword;
typedef uint32_t	Elf64_Word;
typedef uint64_t	Elf64_Lword;
typedef uint64_t	Elf64_Xword;
typedef Elf64_Half Elf64_Section;

typedef struct Elf64_Phdr
{
  Elf64_Word    p_type;         /* Segment type */
  Elf64_Word    p_flags;        /* Segment flags */
  Elf64_Off     p_offset;       /* Segment file offset */
  Elf64_Addr    p_vaddr;        /* Segment virtual address */
  Elf64_Addr    p_paddr;        /* Segment physical address */
  Elf64_Xword   p_filesz;       /* Segment size in file */
  Elf64_Xword   p_memsz;        /* Segment size in memory */
  Elf64_Xword   p_align;        /* Segment alignment */
} Elf64_Phdr;

#define EI_MAG0   0       /* File identification byte 0 index */
#define EI_MAG1   1       /* File identification byte 1 index */
#define EI_MAG2   2       /* File identification byte 2 index */
#define EI_MAG3   3       /* File identification byte 3 index */
#define EI_CLASS  4       /* File class index */
#define EI_DATA   5       /* Data encoding index */
#define EI_VERSION 6      /* File version index */
#define EI_OSABI  7       /* OS ABI identification */
#define EI_ABIVERSION 8   /* ABI version */
#define EI_PAD    9       /* Start of padding bytes */
#define EI_NIDENT 16      /* Size of e_ident[] */


typedef struct Elf64_Ehdr {
  unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
  Elf64_Half    e_type;         /* Object file type */
  Elf64_Half    e_machine;      /* Architecture */
  Elf64_Word    e_version;      /* Object file version */
  Elf64_Addr    e_entry;        /* Entry point virtual address */
  Elf64_Off     e_phoff;        /* Program header table file offset */
  Elf64_Off     e_shoff;        /* Section header table file offset */
  Elf64_Word    e_flags;        /* Processor-specific flags */
  Elf64_Half    e_ehsize;       /* ELF header size in bytes */
  Elf64_Half    e_phentsize;    /* Program header table entry size */
  Elf64_Half    e_phnum;        /* Program header table entry count */
  Elf64_Half    e_shentsize;    /* Section header table entry size */
  Elf64_Half    e_shnum;        /* Section header table entry count */
  Elf64_Half    e_shstrndx;     /* Section header string table index */
} Elf64_Ehdr;

typedef struct Elf64_Shdr {
  Elf64_Word    sh_name;        /* Section name (string tbl index) */
  Elf64_Word    sh_type;        /* Section type */
  Elf64_Xword   sh_flags;       /* Section flags */
  Elf64_Addr    sh_addr;        /* Section virtual addr at execution */
  Elf64_Off     sh_offset;      /* Section file offset */
  Elf64_Xword   sh_size;        /* Section size in bytes */
  Elf64_Word    sh_link;        /* Link to another section */
  Elf64_Word    sh_info;        /* Additional section information */
  Elf64_Xword   sh_addralign;   /* Section alignment */
  Elf64_Xword   sh_entsize;     /* Entry size if section holds table */
} Elf64_Shdr;

typedef struct Elf64_Dyn
{
  Elf64_Sxword  d_tag;          /* Dynamic entry type */
  union
    {
      Elf64_Xword d_val;        /* Integer value */
      Elf64_Addr d_ptr;         /* Address value */
    } d_un;
} Elf64_Dyn;

/* Legal values for d_tag (dynamic entry type).  */
#define DT_NULL     0       /* Marks end of dynamic section */
#define DT_NEEDED   1       /* Name of needed library */
#define DT_PLTRELSZ 2       /* Size in bytes of PLT relocs */
#define DT_PLTGOT   3       /* Processor defined value */
#define DT_HASH     4       /* Address of symbol hash table */
#define DT_STRTAB   5       /* Address of string table */
#define DT_SYMTAB   6       /* Address of symbol table */
#define DT_RELA     7       /* Address of Rela relocs */
#define DT_RELASZ   8       /* Total size of Rela relocs */
#define DT_RELAENT  9       /* Size of one Rela reloc */
#define DT_STRSZ    10      /* Size of string table */
#define DT_SYMENT   11      /* Size of one symbol table entry */
#define DT_INIT     12      /* Address of init function */
#define DT_FINI     13      /* Address of termination function */
#define DT_SONAME   14      /* Name of shared object */
#define DT_RPATH    15      /* Library search path (deprecated) */
#define DT_SYMBOLIC 16      /* Start symbol search here */
#define DT_REL      17      /* Address of Rel relocs */
#define DT_RELSZ    18      /* Total size of Rel relocs */
#define DT_RELENT   19      /* Size of one Rel reloc */
#define DT_PLTREL   20      /* Type of reloc in PLT */
#define DT_DEBUG    21      /* For debugging; unspecified */
#define DT_TEXTREL  22      /* Reloc might modify .text */
#define DT_JMPREL   23      /* Address of PLT relocs */
#define DT_BIND_NOW 24      /* Process relocations of object */
#define DT_INIT_ARRAY   25      /* Array with addresses of init fct */
#define DT_FINI_ARRAY   26      /* Array with addresses of fini fct */
#define DT_INIT_ARRAYSZ 27      /* Size in bytes of DT_INIT_ARRAY */
#define DT_FINI_ARRAYSZ 28      /* Size in bytes of DT_FINI_ARRAY */
#define DT_RUNPATH  29      /* Library search path */
#define DT_FLAGS    30      /* Flags for the object being loaded */
#define DT_ENCODING 32      /* Start of encoded range */
#define DT_PREINIT_ARRAY 32     /* Array with addresses of preinit fct*/
#define DT_PREINIT_ARRAYSZ 33       /* size in bytes of DT_PREINIT_ARRAY */
#define DT_SYMTAB_SHNDX 34      /* Address of SYMTAB_SHNDX section */
#define DT_NUM      35      /* Number used */
#define DT_LOOS     0x6000000d  /* Start of OS-specific */
#define DT_HIOS     0x6ffff000  /* End of OS-specific */
#define DT_LOPROC   0x70000000  /* Start of processor-specific */
#define DT_HIPROC   0x7fffffff  /* End of processor-specific */
#define DT_PROCNUM  DT_MIPS_NUM /* Most used by any processor */


typedef struct
{
  Elf64_Word    st_name;        /* Symbol name (string tbl index) */
  unsigned char st_info;        /* Symbol type and binding */
  unsigned char st_other;       /* Symbol visibility */
  Elf64_Section st_shndx;       /* Section index */
  Elf64_Addr    st_value;       /* Symbol value */
  Elf64_Xword   st_size;        /* Symbol size */
} Elf64_Sym;

typedef struct
{
  Elf64_Half si_boundto;        /* Direct bindings, symbol bound to */
  Elf64_Half si_flags;          /* Per symbol flags */
} Elf64_Syminfo;

/* Possible values for si_boundto.  */
#define SYMINFO_BT_SELF       0xffff  /* Symbol bound to self */
#define SYMINFO_BT_PARENT     0xfffe  /* Symbol bound to parent */
#define SYMINFO_BT_LOWRESERVE 0xff00  /* Beginning of reserved entries */

/* Possible bitmasks for si_flags.  */
#define SYMINFO_FLG_DIRECT    0x0001  /* Direct bound symbol */
#define SYMINFO_FLG_PASSTHRU  0x0002  /* Pass-thru symbol for translator */
#define SYMINFO_FLG_COPY      0x0004  /* Symbol is a copy-reloc */
#define SYMINFO_FLG_LAZYLOAD  0x0008  /* Symbol bound to object to be lazy loaded */
/* Syminfo version values.  */
#define SYMINFO_NONE        0
#define SYMINFO_CURRENT     1
#define SYMINFO_NUM         2

/* Both Elf32_Sym and Elf64_Sym use the same one-byte st_info field.  */
#define ELF64_ST_BIND(val)      ELF32_ST_BIND (val)
#define ELF64_ST_TYPE(val)      ELF32_ST_TYPE (val)
#define ELF64_ST_INFO(bind, type)   ELF32_ST_INFO ((bind), (type))

/* Legal values for ST_BIND subfield of st_info (symbol binding).  */
#define STB_LOCAL   0       /* Local symbol */
#define STB_GLOBAL  1       /* Global symbol */
#define STB_WEAK    2       /* Weak symbol */
#define STB_NUM     3       /* Number of defined types.  */
#define STB_LOOS    10      /* Start of OS-specific */
#define STB_GNU_UNIQUE  10  /* Unique symbol.  */
#define STB_HIOS    12      /* End of OS-specific */
#define STB_LOPROC  13      /* Start of processor-specific */
#define STB_HIPROC  15      /* End of processor-specific */

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */
#define STT_NOTYPE  0       /* Symbol type is unspecified */
#define STT_OBJECT  1       /* Symbol is a data object */
#define STT_FUNC    2       /* Symbol is a code object */
#define STT_SECTION 3       /* Symbol associated with a section */
#define STT_FILE    4       /* Symbol's name is file name */
#define STT_COMMON  5       /* Symbol is a common data object */
#define STT_TLS     6       /* Symbol is thread-local data object*/
#define STT_NUM     7       /* Number of defined types.  */
#define STT_LOOS    10      /* Start of OS-specific */
#define STT_GNU_IFUNC   10  /* Symbol is indirect code object */
#define STT_HIOS    12      /* End of OS-specific */
#define STT_LOPROC  13      /* Start of processor-specific */
#define STT_HIPROC  15      /* End of processor-specific */

/* Symbol table indices are found in the hash buckets and chain table
   of a symbol hash table section.  This special index value indicates
   the end of a chain, meaning no further symbols are found in that bucket.  */
#define STN_UNDEF   0       /* End of a chain.  */


/* How to extract and insert information held in the st_other field.  */
#define ELF32_ST_VISIBILITY(o)  ((o) & 0x03)

/* For ELF64 the definitions are the same.  */
#define ELF64_ST_VISIBILITY(o)  ELF32_ST_VISIBILITY (o)

/* Symbol visibility specification encoded in the st_other field.  */
#define STV_DEFAULT   0    /* Default symbol visibility rules */
#define STV_INTERNAL  1    /* Processor specific hidden class */
#define STV_HIDDEN    2    /* Sym unavailable in other modules */
#define STV_PROTECTED 3    /* Not preemptible, not exported */

#define ELF64_R_SYM(i)          ((i) >> 32)
#define ELF64_R_TYPE(i)         ((i) & 0xffffffff)
#define ELF64_R_INFO(sym,type)  ((((Elf64_Xword) (sym)) << 32) + (type))

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
        // printf("Invalid class (%u)\n", header.e_ident[EI_CLASS]);
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
        // printf("Invalid data encoding (%u)\n", header.e_ident[EI_DATA]);
        return false;
    }

    // Confirm version is valid
    if (header.e_ident[EI_VERSION] != 1) { // EV_CURRENT
        // printf("Invalid version (0=EV_NONE)\n");
        return false;
    }

    // Check the OS ABI
    char *os_abi = elf_get_os_abi(header);
    if (!os_abi) {
        // printf("Invalid OS ABI\n");
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
            // printf("Invalid memsize\n");
            return false;
        }

        // Confirm that the alignment is a power of 2
        if ((header.p_align & (header.p_align - 1)) != 0) {
            // printf("Invalid alignment\n");
            return false;
        }

        // Confirm that the alignment is not zero
        if (header.p_align == 0) {
            // printf("Invalid alignment\n");
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
            // printf("OS-specific\n");
        } else if (header.p_type >= PT_LOPROC && header.p_type <= PT_HIPROC) {
            // printf("Processor-specific\n");
        } else {
            // printf("(unrecognized/invalid)\n");
            return false;
        }
        break;
    }
    return true;
}

#ifdef __STDC_HOSTED__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void elf_debug_header(Elf64_Ehdr header) {
    if (!elf_is_valid_header(header)) {
        printf("Invalid ELF header\n");
        return;
    }
    printf("ELF Header:\n");
    printf("   Magic:   ");
    for (int i = 0; i < EI_NIDENT; i++) {
        printf("%02x ", header.e_ident[i]);
    }

    printf("\n");
    printf("   Class:                              ");
    switch (header.e_ident[EI_CLASS]) {
    case 0:
        printf("(invalid)\n");
        break;
    case 1:
        printf("ELF 32-bit\n");
        break;
    case 2:
        printf("ELF 64-bit\n");
        break;
    default:
        printf("(unrecognized/invalid)\n");
        break;
    }
    printf("   Data:                               ");
    switch (header.e_ident[EI_DATA]) {
    case 1:
        printf("(little endian)\n");
        break;
    case 2:
        printf("(big endian)\n");
        break;
    default:
        printf("(invalid)\n");
        break;
    }
    printf("   Version:                            ");
    switch (header.e_ident[EI_VERSION]) {
    case 0:
        printf("(invalid)\n");
        break;
    case 1:
        printf("(current)\n");
        break;
    default:
        printf("(unrecognized/invalid)\n");
        break;
    }
    printf("   OS/ABI:                             ");
    char *os_abi = elf_get_os_abi(header);
    if (os_abi) {
        printf("(%s)\n", os_abi);
    } else {
        printf("(invalid)\n");
    }
    printf("   ABI Version:                        %u\n", header.e_ident[EI_ABIVERSION]);
    printf("   Type:                               ");
    switch (header.e_type) {
    case ET_NONE:
        printf("None\n");
        break;
    case ET_REL:
        printf("Relocatable file\n");
        break;
    case ET_EXEC:
        printf("Executable file\n");
        break;
    case ET_DYN:
        printf("Shared object file\n");
        break;
    case ET_CORE:
        printf("Core file\n");
        break;
    case ET_LOOS:
        printf("Operating system-specific (LOOS)\n");
        break;
    case ET_HIOS:
        printf("Operating system-specific (HIOS)\n");
        break;
    case ET_LOPROC:
        printf("Processor-specific (LOPROC)\n");
        break;
    case ET_HIPROC:
        printf("Processor-specific (HIPROC)\n");
        break;
    default:
        printf("(unrecognized/invalid)\n");
        break;
    }

    printf("   Machine:                            ");
    char *machine_name = elf_get_machine_name(header);
    if (machine_name) {
        printf("%s\n", machine_name);
    } else {
        printf("(unrecognized/invalid)\n");
    }

    printf("   Version:                            %u\n", header.e_version);
    printf("   Entry point address:                0x%lx\n", header.e_entry);
    printf("   Start of program headers:           0x%lx\n", header.e_phoff);
    printf("   Start of section headers:           0x%lx\n", header.e_shoff);
    printf("   Flags:                              0x%x\n", header.e_flags);
    printf("   Size of this header:                %u\n", header.e_ehsize);
    printf("   Size of program headers:            %u\n", header.e_phentsize);
    printf("   Number of program headers:          %u\n", header.e_phnum);
    printf("   Size of section headers:            %u\n", header.e_shentsize);
    printf("   Number of section headers:          %u\n", header.e_shnum);
    printf("   Section header string table index:  %u\n", header.e_shstrndx);
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

void elf_debug_program_header(Elf64_Phdr header) {
    printf("Program Header:\n");
    // printf("   Type:             0x%x\n", header.p_type);
    printf("   Type:             ");
    switch (header.p_type) {
    case PT_NULL:
        printf("Unused entry\n");
        break;
    case PT_LOAD:
        printf("Loadable segment\n");
        break;
    case PT_DYNAMIC:
        printf("Dynamic linking information\n");
        break;
    case PT_INTERP:
        printf("Interpreter information\n");
        break;
    case PT_NOTE:
        printf("Auxiliary information\n");
        break;
    case PT_SHLIB:
        printf("Reserved\n");
        break;
    case PT_PHDR:
        printf("Segment containing program header table itself\n");
        break;
    case PT_TLS:
        printf("Thread-local storage segment\n");
        break;
    case PT_LOOS:
        printf("Start of OS-specific\n");
        break;
    case PT_HIOS:
        printf("End of OS-specific\n");
        break;
    case PT_LOPROC:
        printf("Start of processor-specific\n");
        break;
    case PT_HIPROC:
        printf("End of processor-specific\n");
        break;
    default:
        if (header.p_type >= PT_LOOS && header.p_type <= PT_HIOS) {
            printf("OS-specific\n");
        } else if (header.p_type >= PT_LOPROC && header.p_type <= PT_HIPROC) {
            printf("Processor-specific\n");
        } else {
            printf("(unrecognized/invalid)\n");
        }
        break;
    }

    printf("   Flags:            ");
    if (header.p_flags & PF_R) {
        printf("R");
    }
    if (header.p_flags & PF_W) {
        printf("W");
    }
    if (header.p_flags & PF_X) {
        printf("X");
    }
    if (header.p_flags & ~(PF_R | PF_W | PF_X)) {
        printf("(unrecognized/invalid)");
    }
    printf(" ( ");
    if (header.p_flags & PF_R) {
        printf("Read ");
    }
    if (header.p_flags & PF_W) {
        printf("Write ");
    }
    if (header.p_flags & PF_X) {
        printf("Exec ");
    }
    printf(")");
    

    printf("\n");
    printf("   Offset:           0x%lx\n", header.p_offset);
    printf("   Virtual Address:  0x%lx\n", header.p_vaddr);
    printf("   Physical Address: 0x%lx\n", header.p_paddr);
    printf("   File Size:        0x%lx\n", header.p_filesz);
    printf("   Memory Size:      0x%lx\n", header.p_memsz);
    printf("   Alignment:        0x%lx\n", header.p_align);
}

uint64_t get_file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    uint64_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    return fsize;
}

uint8_t *get_file_as_buffer(FILE *f) {
    uint64_t fsize = get_file_size(f);
    uint8_t *buffer = malloc(fsize);
    fread(buffer, fsize, 1, f);
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    char *input_file = argv[1];

    FILE *f = fopen(input_file, "rb");
    if (!f) {
        printf("Error: could not open file '%s'\n", input_file);
        return 1;
    }

    uint8_t *buffer = get_file_as_buffer(f);

    // Read the ELF header
    Elf64_Ehdr header;
    memcpy(&header, buffer, sizeof(header));
    if (!elf_is_valid_header(header)) {
        printf("Invalid ELF header\n");
        return 1;
    }
    elf_debug_header(header);
    
    // Read the program headers
    Elf64_Phdr *program_headers = malloc(header.e_phentsize * header.e_phnum);
    memcpy(program_headers, buffer + header.e_phoff, header.e_phentsize * header.e_phnum);
    for (uint32_t i = 0; i < header.e_phnum; i++) {
        if (!elf_is_valid_program_header(program_headers[i])) {
            printf("Invalid program header #%u\n", i);
        }

        elf_debug_program_header(program_headers[i]);
    }
}
#endif