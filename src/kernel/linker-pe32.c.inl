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
#ifndef __KOS_KERNEL_LINKER_PE32_C_INL__
#define __KOS_KERNEL_LINKER_PE32_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/errno.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/syslog.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/shm.h>
#include <sys/types.h>
#include <windows/pe.h>
#include <math.h>
#include <stddef.h>

#include "linker-pe32.h"

__DECL_BEGIN


__crit kerrno_t
ksecdata_pe32_init(struct ksecdata *__restrict self,
                   Pe32_SectionHeader const *__restrict headerv,
                   size_t headerc, struct kfile *__restrict pe_file,
                   uintptr_t image_base) {
 struct kshlibsection *iter; struct kshmregion *region;
 Pe32_SectionHeader const *head_iter,*head_end;
 size_t usable_headers = 0; kerrno_t error = KE_OK;
 size_t pages,filesize; kshm_flag_t flags;
 kassertobj(self);
 head_end = (head_iter = headerv)+headerc;
 for (; head_iter != head_end; ++head_iter) {
  if (head_iter->Misc.VirtualSize) ++usable_headers;
 }
 self->ed_secc = usable_headers;
 if (!usable_headers) { self->ed_secv = NULL; goto done; }
 self->ed_secv = (struct kshlibsection *)malloc(usable_headers*sizeof(struct kshlibsection));
 if __unlikely(!self->ed_secv) return KE_NOMEM;
 iter = self->ed_secv;
 head_end = (head_iter = headerv)+headerc;
 for (; head_iter != head_end; ++head_iter) {
  if (head_iter->Misc.VirtualSize) {
   assert(iter != self->ed_secv+self->ed_secc);
   pages = ceildiv(head_iter->Misc.VirtualSize,PAGESIZE);
   flags = KSHMREGION_FLAG_NONE;
#define CHARACTERISTIC(f) ((head_iter->Characteristics&(f))==(f))
   if (CHARACTERISTIC(IMAGE_SCN_MEM_SHARED))  flags |= KSHMREGION_FLAG_SHARED;
   if (CHARACTERISTIC(IMAGE_SCN_MEM_EXECUTE)) flags |= KSHMREGION_FLAG_EXEC;
   if (CHARACTERISTIC(IMAGE_SCN_MEM_READ))    flags |= KSHMREGION_FLAG_READ;
   if (CHARACTERISTIC(IMAGE_SCN_MEM_WRITE))   flags |= KSHMREGION_FLAG_WRITE;
#undef CHARACTERISTIC
   /* Allocate a region big enough to support this section. */
   region = kshmregion_newram(pages,flags);
   if __unlikely(!region) { error = KE_NOMEM; goto err_iter; }
   iter->sls_region = region;
   iter->sls_size   = head_iter->Misc.VirtualSize;
   iter->sls_base   = image_base+(ksymaddr_t)head_iter->VirtualAddress;
   iter->sls_albase = alignd(iter->sls_base,PAGEALIGN);
   filesize         = head_iter->SizeOfRawData;
   if __unlikely(filesize > iter->sls_size) filesize = iter->sls_size;
   iter->sls_filebase = (ksymaddr_t)head_iter->PointerToRawData;
   if (!iter->sls_filebase) filesize = 0;
   iter->sls_filesize = filesize;
   /* Actually load the section from disk. */
   error = kshmregion_ploaddata(region,iter->sls_base-iter->sls_albase,
                                pe_file,filesize,iter->sls_filebase);
   if __unlikely(KE_ISERR(error)) goto err_region;
   k_syslogf_prefixfile(KLOG_INFO,pe_file
                       ,"Loading section %.*q from %.8I64u to %.8p+%.8Iu...%.8p (%c%c%c%c)\n"
                       ,(unsigned)IMAGE_SIZEOF_SHORT_NAME,(char *)head_iter->Name
                       ,(__u64)iter->sls_filebase
                       ,(uintptr_t)iter->sls_base,(size_t)iter->sls_size
                       ,(uintptr_t)iter->sls_base+(size_t)iter->sls_size
                       ,(kshmregion_getflags(region)&KSHMREGION_FLAG_SHARED) ? 'S' : '-'
                       ,(kshmregion_getflags(region)&KSHMREGION_FLAG_READ) ? 'R' : '-'
                       ,(kshmregion_getflags(region)&KSHMREGION_FLAG_WRITE) ? 'W' : '-'
                       ,(kshmregion_getflags(region)&KSHMREGION_FLAG_EXEC) ? 'X' : '-');
   ++iter;
  }
 }
 assert(iter == self->ed_secv+self->ed_secc);
done:
 ksecdata_cacheminmax(self);
 return error;
err_region: kshmregion_decref_full(region);
err_iter:
 while (iter-- != self->ed_secv) {
  kshmregion_decref_full(iter->sls_region);
 }
 free(self->ed_secv);
 return error;
}




__crit kerrno_t
kshlib_pe32_new(struct kshlib **__restrict result,
                struct kfile *__restrict pe_file) {
 struct kshlib *lib; kerrno_t error;
 Pe32_DosHeader dos_header;
 Pe32_FileHeader file_header;
 Pe32_SectionHeader *section_headers;
 pos_t section_headers_start;
 uintptr_t image_base;
 kassertobj(result);
 kassert_kfile(pe_file);
 /* Win32 PE executables. */
 if __unlikely((lib = omalloc(struct kshlib)) == NULL) return KE_NOMEM;
 kobject_init(lib,KOBJECT_MAGIC_SHLIB);
 lib->sh_refcnt = 1;
 lib->sh_flags = KSHLIB_FLAG_NONE|KSHLIB_FLAG_FIXED;
 error = kfile_kernel_readall(pe_file,&dos_header,sizeof(dos_header));
 if __unlikely(KE_ISERR(error)) goto err_lib;
 /* Validate that this is really a PE executable. */
 if __unlikely(dos_header.e_magic != PE_DOSHEADER_MAGIC) { error = KE_NOEXEC; goto err_lib; }
 error = kfile_incref(lib->sh_file = pe_file);
 if __unlikely(KE_ISERR(error)) goto err_lib;
 error = kshlibcache_addlib(lib);
 if __unlikely(KE_ISERR(error)) {
  lib->sh_cidx = (__size_t)-1;
  k_syslogf_prefixfile(KLOG_WARN,pe_file,
                       "[LINKER] Failed to cache library file\n");
 }

 /* Load the file header post the 16-bit realmode code. */
 error = kfile_seek(pe_file,dos_header.e_lfanew,SEEK_SET,NULL);
 if __unlikely(KE_ISERR(error)) goto err_cache;
 error = kfile_kernel_readall(pe_file,&file_header,offsetof(Pe32_FileHeader,OptionalHeader));
 if __unlikely(KE_ISERR(error)) goto err_cache;
#define HAS_OPTION(x) \
 (file_header.FileHeader.SizeOfOptionalHeader >= offsetafter(IMAGE_OPTIONAL_HEADER32,x))
 if (file_header.FileHeader.SizeOfOptionalHeader) {
  if (file_header.FileHeader.SizeOfOptionalHeader > sizeof(file_header.OptionalHeader))
      file_header.FileHeader.SizeOfOptionalHeader = sizeof(file_header.OptionalHeader);
  error = kfile_kernel_readall(pe_file,&file_header.OptionalHeader,
                               file_header.FileHeader.SizeOfOptionalHeader);
  if __unlikely(KE_ISERR(error)) goto err_cache;
 }

 image_base = HAS_OPTION(ImageBase) ? file_header.OptionalHeader.ImageBase : 0;
 if (HAS_OPTION(AddressOfEntryPoint)) {
  lib->sh_callbacks.slc_start = (ksymaddr_t)file_header.OptionalHeader.AddressOfEntryPoint;
 } else {
  k_syslogf_prefixfile(KLOG_WARN,pe_file,"[LINKER] Pe binary file has no entry point (Optional header size: %Ix)\n",
                      (size_t)file_header.FileHeader.SizeOfOptionalHeader);
  lib->sh_callbacks.slc_start = (ksymaddr_t)0; /* ??? */
 }
 lib->sh_callbacks.slc_start += image_base;
 //lib->sh_callbacks.slc_start  = 0x00401090;
 k_syslogf_prefixfile(KLOG_DEBUG,pe_file,"[LINKER] Determined entry pointer %p\n",
                      lib->sh_callbacks.slc_start);
 lib->sh_callbacks.slc_init            = KSYM_INVALID;
 lib->sh_callbacks.slc_fini            = KSYM_INVALID;
 lib->sh_callbacks.slc_preinit_array_v = KSYM_INVALID;
 lib->sh_callbacks.slc_preinit_array_c = 0;
 lib->sh_callbacks.slc_init            = KSYM_INVALID;
 lib->sh_callbacks.slc_init_array_v    = KSYM_INVALID;
 lib->sh_callbacks.slc_init_array_c    = 0;
 lib->sh_callbacks.slc_fini            = KSYM_INVALID;
 lib->sh_callbacks.slc_fini_array_v    = KSYM_INVALID;
 lib->sh_callbacks.slc_fini_array_c    = 0;

 ksymtable_init(&lib->sh_publicsym);
 ksymtable_init(&lib->sh_weaksym);
 ksymtable_init(&lib->sh_privatesym);

 /* TODO: Dependencies. */
 kshliblist_init(&lib->sh_deps);

 /* Allocate temporary memory for all section headers. */
 section_headers_start = dos_header.e_lfanew+
                         offsetof(Pe32_FileHeader,OptionalHeader)+
                         file_header.FileHeader.SizeOfOptionalHeader;
 section_headers = (Pe32_SectionHeader *)malloc(file_header.FileHeader.NumberOfSections*
                                                sizeof(Pe32_SectionHeader));
 if __unlikely(!section_headers) { error = KE_NOMEM; goto err_cache; }
 error = kfile_kernel_preadall(pe_file,section_headers_start,section_headers,
                               file_header.FileHeader.NumberOfSections*
                               sizeof(Pe32_SectionHeader));
 if __unlikely(KE_ISERR(error)) goto err_secheaders;
 error = ksecdata_pe32_init(&lib->sh_data,section_headers,
                            file_header.FileHeader.NumberOfSections,
                            pe_file,image_base);
 free(section_headers),section_headers = NULL; /* Set to NULL to trick cleanup code. */
 if __unlikely(KE_ISERR(error)) goto err_secheaders;
 kreloc_init(&lib->sh_reloc);

 /* TODO: Relocations. */

#undef HAS_OPTION
 lib->sh_flags |= KSHLIB_FLAG_LOADED;
 *result = lib;
 return error;
//err_reloc: kreloc_quit(&lib->sh_reloc);
err_secheaders: free(section_headers);
 kshliblist_quit(&lib->sh_deps);
 ksymtable_quit(&lib->sh_privatesym);
 ksymtable_quit(&lib->sh_weaksym);
 ksymtable_quit(&lib->sh_publicsym);
err_cache: kshlibcache_dellib(lib);
/*err_file:*/ kfile_decref(lib->sh_file);
err_lib: free(lib);
 return error;
}


__DECL_END

#endif /* !__KOS_KERNEL_LINKER_PE32_C_INL__ */
