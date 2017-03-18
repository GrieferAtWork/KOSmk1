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
#ifndef __KOS_KERNEL_LINKER_SLLOADERS_C_INL__
#define __KOS_KERNEL_LINKER_SLLOADERS_C_INL__ 1

#include <kos/kernel/types.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/fs/file.h>
#include <kos/errno.h>
#include <kos/syslog.h>
#include <windows/pe.h>

__DECL_BEGIN

typedef kerrno_t (*pslloader)(struct kshlib **__restrict result,
                              struct kfile *__restrict file);
#define SLLOADER_MAX_MAGIC  4 /*< No supported type has more than 4 bytes of magic. */
struct slloader {
 pslloader sl_callback; /*< [1..1] Loader function callback. */
 __u8      sl_magic[SLLOADER_MAX_MAGIC];
 __size_t  sl_magicsz; /*< Use magic size. */
};

static struct slloader slloaders[] = {
 /* ELF32   */{&kshlib_elf32_new,  {ELFMAG0,ELFMAG1,ELFMAG2,ELFMAG3},4},
 /* PE-32   */{&kshlib_pe32_new,   {PE_DOSHEADER_MAGIC_1,PE_DOSHEADER_MAGIC_2},2},
 /* Shebang */{&kshlib_shebang_new,{'#','!'},2},
 {NULL,{0,},0}, // Sentinal
};

__crit kerrno_t kshlib_new(struct kshlib **__restrict result,
                           struct kfile *__restrict exe_file) {
 char magic[SLLOADER_MAX_MAGIC];
 size_t magic_max; struct slloader *loader = slloaders;
 kerrno_t error;
 KTASK_CRIT_MARK
 error = kfile_kernel_read(exe_file,magic,sizeof(magic),&magic_max);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfile_seek(exe_file,-(__off_t)sizeof(magic),SEEK_CUR,NULL); // rewind the file.
 if __unlikely(KE_ISERR(error)) return error;
 // Search for a loader matching the recognized magic.
 while (loader->sl_callback) {
  if (magic_max >= loader->sl_magicsz &&
      memcmp(magic,loader->sl_magic,loader->sl_magicsz) == 0
      ) return (*loader->sl_callback)(result,exe_file);
  ++loader;
 }
 k_syslogf_prefixfile(KLOG_WARN,exe_file
                     ,"Unrecognized magic: %.2I8x%.2I8x%.2I8x%.2I8x\n"
                     ,magic_max >= 1 ? magic[0] : 0
                     ,magic_max >= 2 ? magic[1] : 0
                     ,magic_max >= 3 ? magic[2] : 0
                     ,magic_max >= 4 ? magic[3] : 0);
 return KE_NOEXEC;
}

__DECL_END

#endif /* !__KOS_KERNEL_LINKER_SLLOADERS_C_INL__ */
