#pragma once
/* This file defines standard ELF types, structures, and macros.
   Copyright (C) 1995-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* Standard ELF types.  */
#include <stdbool.h>
#include <kmalloc.h>
#include <util.h>
#include <stdint.h>
#include <process.h>
#include <virtio.h>

struct process;
/**
 * @brief Load an ELF file into a process and update its fields.
 *
 * @param p The process to load the ELF into.
 * @param elf A buffer of the ELF file. This must be canonical!
 * @return true The ELF file was read and the process updated.
 * @return false The ELF file is not valid, or isn't targeting our machine.
 */
bool elf_load(struct process *p, const void *elf);


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

bool elf_is_little_endian(Elf64_Ehdr header);

bool elf_is_big_endian(Elf64_Ehdr header);

char *elf_get_machine_name(Elf64_Ehdr header);

char *elf_get_os_abi(Elf64_Ehdr header);

bool elf_is_valid_header(Elf64_Ehdr header);

bool elf_is_valid_program_header(Elf64_Phdr header);

void elf_debug_header(Elf64_Ehdr header);

void elf_debug_program_header(Elf64_Phdr header);

int elf_create_process(Process *p, const uint8_t *elf);

int elf_load_process(Process *p, const uint8_t *elf);

int elf_load_file(const uint8_t *elf, uint64_t *entry_point);

bool elf_is_valid(const uint8_t *elf);

int elf_load_block_device(VirtioDevice *block_device, uint64_t offset, uint64_t *entry_point);