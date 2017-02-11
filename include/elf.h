/* MIT License
 *
 * Copyright (c) 2017 GrieferAtWork
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __ELF_H__
#ifndef _ELF_H
#define __ELF_H__ 1
#define _ELF_H 1

#include <kos/compiler.h>
#include <kos/types.h>

#ifndef __ASSEMBLY__
__DECL_BEGIN
__COMPILER_PACK_PUSH(1)

typedef __u16      Elf32_Half;
typedef __u32      Elf32_Word;
typedef __s32      Elf32_Sword;
typedef __u64      Elf32_Xword;
typedef __s64      Elf32_Sxword;
typedef __u32      Elf32_Addr;
typedef __u32      Elf32_Off;
typedef __u16      Elf32_Section;
typedef Elf32_Half Elf32_Versym;

#define EI_NIDENT (16)

#define ELFMAG0		0x7F
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'

typedef struct __packed {
union __packed {
struct __packed {
    unsigned char ehrd_magic[4];
    __u8          ehrd_kind;       /*< 1: 32-bit; 2: 64-bit. */
    __u8          ehrd_endian;     /*< 1: little endian; 2: big endian. */
    __u8          ehrd_version;    /*< ELF Version. */
    __u8          ehrd_osabi;      /*< OS ABI (usually 0 for System V). */
    __u8          ehrd_padding[8]; /*< Unused/Padding. */
};
    unsigned char e_ident[EI_NIDENT]; /*< Magic number and other info. */
};
    Elf32_Half    e_type;             /*< Object file type. */
    Elf32_Half    e_machine;          /*< Architecture. */
    Elf32_Word    e_version;          /*< Object file version. */
    Elf32_Addr    e_entry;            /*< Entry point virtual address. */
    Elf32_Off     e_phoff;            /*< Program header table file offset. */
    Elf32_Off     e_shoff;            /*< Section header table file offset. */
    Elf32_Word    e_flags;            /*< Processor-specific flags. */
    Elf32_Half    e_ehsize;           /*< ELF header size in bytes. */
    Elf32_Half    e_phentsize;        /*< Program header table entry size. */
    Elf32_Half    e_phnum;            /*< Program header table entry count. */
    Elf32_Half    e_shentsize;        /*< Section header table entry size. */
    Elf32_Half    e_shnum;            /*< Section header table entry count. */
    Elf32_Half    e_shstrndx;         /*< Section header string table index. */
} Elf32_Ehdr;

// Legal values for e_type (object file type).
#define ET_NONE 0 /*< No file type. */
#define ET_REL  1 /*< Relocatable file. */
#define ET_EXEC 2 /*< Executable file. */
#define ET_DYN  3 /*< Shared object file. */
// #define ET_CORE 4 /*< Core file. */
// #define ET_NUM  5 /*< Number of defined types. */

// Legal values for e_machine (architecture).
#define EM_386 3 /*< Intel 80386. */


typedef struct __packed {
    Elf32_Word p_type;   /*< Segment type. */
    Elf32_Off  p_offset; /*< Segment file offset. */
    Elf32_Addr p_vaddr;  /*< Segment virtual address. */
    Elf32_Addr p_paddr;  /*< Segment physical address. */
    Elf32_Word p_filesz; /*< Segment size in file. */
    Elf32_Word p_memsz;  /*< Segment size in memory. */
    Elf32_Word p_flags;  /*< Segment flags. */
    Elf32_Word p_align;  /*< Segment alignment. */
} Elf32_Phdr;

// Legal values for p_type (segment type).
#define PT_NULL    0 /*< Program header table entry unused. */
#define PT_LOAD    1 /*< Loadable program segment (p_offset is raw memory to-be loaded). */
#define PT_DYNAMIC 2 /*< Dynamic linking information (p_offset points to 'Elf32_Dyn'). */
#define PT_INTERP  3 /*< Program interpreter. */
#define PT_NOTE    4 /*< Auxiliary information. */
#define PT_SHLIB   5 /*< Reserved. */
#define PT_PHDR    6 /*< Entry for header table itself. */
#define PT_TLS     7 /*< Thread-local storage segment. */
#define	PT_NUM     8 /*< Number of defined types. */

// Legal values for p_flags
#define PF_X 0x01 /*< Segment is executable. */
#define PF_W 0x02 /*< Segment is writable. */
#define PF_R 0x04 /*< Segment is readable. */


typedef struct __packed {
    Elf32_Sword d_tag;
    union __packed {
        Elf32_Word d_val;
        Elf32_Addr d_ptr;
        Elf32_Off  d_off;
    } d_un;
} Elf32_Dyn;

#define DT_NULL            0
#define DT_NEEDED          1
#define DT_PLTRELSZ        2
#define DT_PLTGOT          3
#define DT_HASH            4
#define DT_STRTAB          5
#define DT_SYMTAB          6
#define DT_RELA            7
#define DT_RELASZ          8
#define DT_RELAENT         9
#define DT_STRSZ           10
#define DT_SYMENT          11
#define DT_INIT            12
#define DT_FINI            13
#define DT_SONAME          14
#define DT_RPATH           15
#define DT_SYMBOLIC        16
#define DT_REL             17
#define DT_RELSZ           18
#define DT_RELENT          19
#define DT_PLTREL          20
#define DT_DEBUG           21
#define DT_TEXTREL         22
#define DT_JMPREL          23
#define DT_INIT_ARRAY 	    25
#define DT_FINI_ARRAY 	    26
#define DT_INIT_ARRAYSZ 	  27
#define DT_FINI_ARRAYSZ 	  28
#define DT_ENCODING        32 /*< Collision? */
#define DT_PREINIT_ARRAY 	 32
#define DT_PREINIT_ARRAYSZ 33
#define DT_RELACOUNT       0x6ffffff9
#define DT_RELCOUNT        0x6ffffffa
#define DT_LOOS 	          0x6000000D
#define DT_HIOS 	          0x6ffff000
#define DT_LOPROC          0x70000000
#define DT_HIPROC          0x7fffffff



// Section Header
typedef struct __packed {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off  sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 	 2
#define SHT_STRTAB 	 3
#define SHT_RELA 	   4
#define SHT_HASH 	   5
#define SHT_DYNAMIC 	6
#define SHT_NOTE 	   7
#define SHT_NOBITS 	 8
#define SHT_REL 	    9
#define SHT_SHLIB 	  10
#define SHT_DYNSYM 	 11
#define SHT_LOPROC 	 0x70000000
#define SHT_HIPROC 	 0x7fffffff
#define SHT_LOUSER 	 0x80000000
#define SHT_HIUSER 	 0xffffffff 


typedef struct __packed {
    Elf32_Word    st_name;
    Elf32_Addr    st_value;
    Elf32_Word    st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half    st_shndx;
} Elf32_Sym;

#define SHN_UNDEF 0

#define ELF32_ST_BIND(info)          ((info) >> 4)
#define ELF32_ST_TYPE(info)          ((info) & 0xf)
#define ELF32_ST_INFO(bind, type)    (((bind)<<4)+((type)&0xf))
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2 

typedef struct __packed {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} Elf32_Rel;

typedef struct __packed {
    Elf32_Addr	 r_offset;
    Elf32_Word	 r_info;
    Elf32_Sword	r_addend;
} Elf32_Rela;


#define ELF32_R_SYM(i)    ((i)>>8)
#define ELF32_R_TYPE(i)   ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))


// Taken from '/usr/include/elf.h'

/* i386 relocs.  */
#define R_386_NONE           0  /* No reloc */
#define R_386_32             1  /* Direct 32 bit  */
#define R_386_PC32           2  /* PC relative 32 bit */
#define R_386_GOT32          3  /* 32 bit GOT entry */
#define R_386_PLT32          4  /* 32 bit PLT address */
#define R_386_COPY           5  /* Copy symbol at runtime */
#define R_386_GLOB_DAT       6  /* Create GOT entry */
#define R_386_JMP_SLOT       7  /* Create PLT entry */
#define R_386_RELATIVE       8  /* Adjust by program base */
#define R_386_GOTOFF         9  /* 32 bit offset to GOT */
#define R_386_GOTPC         10  /* 32 bit PC relative offset to GOT */
#define R_386_32PLT         11
#define R_386_TLS_TPOFF     14  /* Offset in static TLS block */
#define R_386_TLS_IE        15  /* Address of GOT entry for static TLS block offset */
#define R_386_TLS_GOTIE     16  /* GOT entry for static TLS block offset */
#define R_386_TLS_LE        17  /* Offset relative to static TLS block */
#define R_386_TLS_GD        18  /* Direct 32 bit for GNU version of general dynamic thread local data */
#define R_386_TLS_LDM       19  /* Direct 32 bit for GNU version of local dynamic thread local data in LE code */
#define R_386_16            20
#define R_386_PC16          21
#define R_386_8             22
#define R_386_PC8           23
#define R_386_TLS_GD_32     24  /* Direct 32 bit for general dynamic thread local data */
#define R_386_TLS_GD_PUSH   25  /* Tag for pushl in GD TLS code */
#define R_386_TLS_GD_CALL   26  /* Relocation for call to __tls_get_addr() */
#define R_386_TLS_GD_POP    27  /* Tag for popl in GD TLS code */
#define R_386_TLS_LDM_32    28  /* Direct 32 bit for local dynamic thread local data in LE code */
#define R_386_TLS_LDM_PUSH  29  /* Tag for pushl in LDM TLS code */
#define R_386_TLS_LDM_CALL  30  /* Relocation for call to __tls_get_addr() in LDM code */
#define R_386_TLS_LDM_POP   31  /* Tag for popl in LDM TLS code */
#define R_386_TLS_LDO_32    32  /* Offset relative to TLS block */
#define R_386_TLS_IE_32     33  /* GOT entry for negated static TLS block offset */
#define R_386_TLS_LE_32     34  /* Negated offset relative to static TLS block */
#define R_386_TLS_DTPMOD32  35  /* ID of module containing symbol */
#define R_386_TLS_DTPOFF32  36  /* Offset in TLS block */
#define R_386_TLS_TPOFF32   37  /* Negated offset in static TLS block */
#define R_386_SIZE32        38  /* 32-bit symbol size */
#define R_386_TLS_GOTDESC   39  /* GOT offset for TLS descriptor.  */
#define R_386_TLS_DESC_CALL 40  /* Marker of call through TLS descriptor for relaxation.  */
#define R_386_TLS_DESC      41  /* TLS descriptor containing pointer to code and to argument, returning the TLS offset for the symbol.  */
#define R_386_IRELATIVE     42  /* Adjust indirectly by program base */
/* Keep this the last entry.  */
#define R_386_NUM           43


__COMPILER_PACK_POP
__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_ELF_H */
#endif /* !__ELF_H__ */
