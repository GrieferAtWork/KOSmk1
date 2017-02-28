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
#ifndef __KOS_KERNEL_LINKER_DWARF_H__
#define __KOS_KERNEL_LINKER_DWARF_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/errno.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/mutex.h>

/* DWARF-compatible addr2line debug interpreter.
 * You'd think there'd be a lot of good information on this on the Internet,
 * but the only ~good~ source I could find is the binutils source for
 * what happens when you type: 'readelf --debug-debug=line ...',
 * which tought me everything I've implement here. */

__DECL_BEGIN

/* Technically these are 128-bit, but we don't have that... */
typedef __s64  leb128_t;
typedef __u64 uleb128_t;

typedef struct __packed {
union __packed {
  __u32	__li_length_32; /* If this is 0xffffffff, a in the file a 64-bit length follows and the header is 64-bit. */
  __u64	li_length;      /* WARNING: In-file this describes the chunk-size after this field, but in-mem it is the chunk-size after this header (aka. at 'li_opcodes'). */
};
  __u16 li_version;
union __packed {
  __u32 __li_prologue_length_32; /*< Only 32-bit if __li_length_32 wasn't 0xffffffff (else: 64-bit). */
  __u64 li_prologue_length;
};
  __u8  li_min_insn_length;
  __u8  li_max_ops_per_insn; /* Missing if li_version < 4 */
  __u8  li_default_is_stmt;
  __s8  li_line_base;
  __u8  li_line_range;
  __u8  li_opcode_base;
//__u8  li_opcodes[li_opcode_base ? li_opcode_base-1 : 0]; /*< Inline list of opcodes and lengths. */
//char  li_dirnames[][];                                   /*< Inline list of directory names, each entry is \0-terminated; last entry is terminated with \0\0. */
// ...
} DWARF2_Internal_LineInfo;

#define DW_LNS_extended_op        0x00
#define DW_LNS_copy               0x01
#define DW_LNS_advance_pc         0x02
#define DW_LNS_advance_line       0x03
#define DW_LNS_set_file           0x04
#define DW_LNS_set_column         0x05
#define DW_LNS_negate_stmt        0x06
#define DW_LNS_set_basic_block    0x07
#define DW_LNS_const_add_pc       0x08
#define DW_LNS_fixed_advance_pc   0x09
#define DW_LNS_set_prologue_end   0x0a
#define DW_LNS_set_epilogue_begin 0x0b
#define DW_LNS_set_isa            0x0c

#define DW_LNE_end_sequence      0x01
#define DW_LNE_set_address       0x02
#define DW_LNE_define_file       0x03
#define DW_LNE_set_discriminator 0x04
#define DW_LNE_lo_user           0x80
#define DW_LNE_hi_user           0xff

struct kdwarf_section;


//////////////////////////////////////////////////////////////////////////
// Parse a signed/unsigned leb integer, advancing
// the pointer in '*pdata' but not exceeding 'end'.
extern __nonnull((1,2))  leb128_t parse_sleb128(__byte_t const **__restrict pdata, __byte_t const *__restrict end);
extern __nonnull((1,2)) uleb128_t parse_uleb128(__byte_t const **__restrict pdata, __byte_t const *__restrict end);


//////////////////////////////////////////////////////////////////////////
// Read a .debug_line header.
// NOTE: '*header_size' is set to the actual size of the header in the file, meaning that
//       any following data can be read from 'section->ds_section_offset+*header_size'.
// @return: KE_OK:    Successfully read the header.
// @return: KE_NOSPC: The section was too small for the header.
// @return: KE_NOSYS: The header referrs to an unsupported version of DWARF.
// @return: * :       Some other, file-specific error.
extern __crit kerrno_t
kshlib_dwarf_readlineinfo(DWARF2_Internal_LineInfo *ili,
                          struct kdwarf_section const *section,
                          struct kfile *__restrict file,
                          size_t *header_size);


struct kdwarf_section {
 __u64 ds_section_offset; /*< Physical, absolute offset of the section int the given file. */
 __u64 ds_section_size;   /*< Physical, absolute size of the section int the given file. */
};
struct ka2ldwarfchunk {
 DWARF2_Internal_LineInfo dc_header;  /*< [lock(:d_lock)] Section header. */
 __size_t                 dc_size;    /*< [lock(:d_lock)] Size of the section data. */
 __byte_t                *dc_data;    /*< [lock(:d_lock)][1..1][owned] Lazily loaded section data. */
 __size_t                 dc_opcodec; /*< [lock(:d_lock)] Amount of known opcodes. */
 __u8                    *dc_opcodev; /*< [lock(:d_lock)][0..d_opcodec] Vector of opcode argument counts (standard_opcodes). */
 __size_t                 dc_dirtabc; /*< [lock(:d_lock)] Amount of directory entires in 'd_dirtabv'. */
 char const              *dc_dirtabv; /*< [lock(:d_lock)][1..1] Directory table base (inline list of \0-terminated strings). */
 __size_t                 dc_filtabc; /*< [lock(:d_lock)] Amount of file entires in 'd_filtabv'. */
 char const              *dc_filtabv; /*< [lock(:d_lock)][1..1] File table base (Real structure is too complicated to describe here; look at). */
union{
 char const              *dc_filend;  /*< [lock(:d_lock)][1..1] End address of the file table. */
 __byte_t const          *dc_code;    /*< [lock(:d_lock)][1..1] Begin of the interpreted bytecode used for translation. */
};
 __byte_t const          *dc_codeend; /*< [lock(:d_lock)][1..1] End of the interpreted bytecode used for translation. */
};

//////////////////////////////////////////////////////////////////////////
// Parse one DWARF a2l chunk starting at the given
// information in 'section_avail', whilst updating
// the section offset and size to pointer after
// the parsed chunk on success.
// @return: KE_OK:    Successfully read the header.
// @return: KE_NOSPC: The section was too small for the header.
// @return: KE_NOSYS: The header refers to an unsupported version of DWARF.
// @return: * :       Some other, file-specific error.
extern __crit __nonnull((1,2,3)) kerrno_t
ka2ldwarfchunk_init(struct ka2ldwarfchunk *__restrict self,
                    struct kfile *__restrict file,
                    struct kdwarf_section *__restrict section_avail);
#define ka2ldwarfchunk_quit(self) free((self)->dc_data)

// Called by 'ka2ldwarfchunk_init':
// Parse the data previously loaded into 'dc_data|dc_size'
extern __crit __nonnull((1,2)) kerrno_t
ka2ldwarfchunk_parse(struct ka2ldwarfchunk *__restrict self,
                     struct kfile *__restrict file);

//////////////////////////////////////////////////////////////////////////
// Execute the interpreted code used for translation of an address.
// @return: KE_NOENT: This chunk does not contain suitable information.
extern __crit __nonnull((1,3)) kerrno_t
ka2ldwarfchunk_exec(struct ka2ldwarfchunk *__restrict self,
                    ksymaddr_t symaddr, struct kfileandline *__restrict result);

struct kdwarffile {
 uleb128_t df_dir;  /*< Index of the associated directory. */
 uleb128_t df_time; /*< Time?. */
 uleb128_t df_size; /*< Size?. */
};

//////////////////////////////////////////////////////////////////////////
// Returns a pointer to the directory name associated with the given index.
// NOTE: The caller must have a lock to 'd_lock' and the engine must be loaded.
// @return: NULL: The given index was out-of-bounds.
extern __wunused __nonnull((1)) char const *
ka2ldwarfchunk_getdir(struct ka2ldwarfchunk const *__restrict self, __size_t index);

//////////////////////////////////////////////////////////////////////////
// Returns a pointer to the file name associated with the given index and
// fills in the field of the given dwarf-file structure pointed to by data.
// NOTE: The caller must have a lock to 'd_lock' and the engine must be loaded.
// @return: NULL: The given index was out-of-bounds.
extern __wunused __nonnull((1,3)) char const *
ka2ldwarfchunk_getfile(struct ka2ldwarfchunk const *__restrict self, __size_t index,
                       struct kdwarffile *__restrict data);




struct ka2ldwarf {
 struct kdwarf_section  d_section; /*< [const] Section location of this engine. */
 struct kmutex          d_lock;    /*< Lock for this engine. */
#define A2L_DWARF_FLAG_NONE 0x00000000
#define A2L_DWARF_FLAG_LOAD 0x00000001 /*< The engine was loaded. */
 __u32                  d_flags;   /*< [lock(d_lock)] Engine flags. */
 size_t                 d_chunkc;  /*< Amount of loaded a2l chunks. */
 struct ka2ldwarfchunk *d_chunkv;  /*< [0..d_chunkc][owned] Vector of A2L chunks. */
};

extern __crit __nonnull((1)) void ka2ldwarf_delete(struct ka2ldwarf *__restrict self);
extern __crit kerrno_t
ka2ldwarf_exec(struct ka2ldwarf *__restrict self,
               struct kshlib *__restrict lib, ksymaddr_t addr,
               struct kfileandline *__restrict result);

//////////////////////////////////////////////////////////////////////////
// Perform lazy loading of debug information.
// NOTE: The caller is responsible for ensuring that 'self->d_lock' is locked.
extern __crit __nonnull((1)) kerrno_t ka2ldwarf_doload_unlocked(struct ka2ldwarf *__restrict self, struct kfile *__restrict file);
extern __crit __nonnull((1)) kerrno_t ka2ldwarf_load_unlocked(struct ka2ldwarf *__restrict self, struct kfile *__restrict file);





//////////////////////////////////////////////////////////////////////////
// Register an available DWARF .debug_line section.
// @return: KE_NOMEM: Not enough available memory to register the dwarf engine.
extern __crit kerrno_t
kshlib_a2l_add_dwarf_debug_line(struct kshlib *__restrict self,
                                __u64 section_offset,
                                __u64 section_size);


__DECL_END

#endif /* !__KOS_KERNEL_LINKER_DWARF_H__ */
