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
#ifndef __KOS_KERNEL_LINKER_ELF32_H__
#define __KOS_KERNEL_LINKER_ELF32_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <elf.h>
#include <stddef.h>

__DECL_BEGIN

struct kfile;
struct kshlib;

static __wunused __nonnull((1,2,3,4)) kerrno_t
kshlib_elf32_load_symtable(struct kshlib *__restrict self,
                           Elf32_Shdr const *__restrict strtab,
                           Elf32_Shdr const *__restrict symtab,
                           struct kfile *__restrict elf_file,
                           int is_dynamic);
__local __wunused __nonnull((1,2,3,5)) kerrno_t
kshlib_elf32_parse_needed(struct kshlib *__restrict self,
                          struct kfile *__restrict elf_file,
                          Elf32_Dyn const *__restrict dynheader, size_t dynheader_c,
                          Elf32_Shdr const *__restrict strtable_header, size_t needed_count);
struct elf32_dynsyminfo {
 Elf32_Shdr dyn_strtable_header;
 Elf32_Shdr dyn_symtable_header;
};

// Initialize 'sh_data' + 'sh_deps' and fill 'dyninfo'
__local __wunused __nonnull((1,2,3,4)) kerrno_t
kshlib_elf32_load_pheaders(struct kshlib *__restrict self,
                           struct elf32_dynsyminfo *__restrict dyninfo,
                           struct kfile *__restrict elf_file,
                           Elf32_Ehdr const *__restrict ehdr);


// Returns 'KS_FOUND' if the given 'data' is inherited
__local __wunused __nonnull((1,2)) kerrno_t
krelocvec_elf32_init(struct krelocvec *__restrict self,
                     Elf32_Rel const *data, size_t entsize,
                     size_t entcount, int is_rela);
__local __wunused __nonnull((1,2,3)) kerrno_t
kshlib_elf32_load_rel(struct kshlib *__restrict self,
                      struct kfile *__restrict elf_file,
                      Elf32_Shdr const *__restrict shdr,
                      int is_rela
#ifdef __DEBUG__
                      , char const *__restrict name
#endif
                      );

__local __wunused __nonnull((1,2,3,4)) kerrno_t
kshlib_elf32_load_sheaders(struct kshlib *__restrict self,
                           struct elf32_dynsyminfo const *__restrict dyninfo,
                           struct kfile *__restrict elf_file,
                           Elf32_Ehdr const *__restrict ehdr);

__DECL_END

#endif /* !__KOS_KERNEL_LINKER_ELF32_H__ */
