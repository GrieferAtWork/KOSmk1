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
#ifndef __KOS_KERNEL_LINKER_C__
#define __KOS_KERNEL_LINKER_C__ 1

#include <assert.h>
#include <malloc.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <kos/errno.h>
#include <kos/types.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/util/string.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/fs/fs.h>

__DECL_BEGIN

static char const default_libsearch_paths[] = "/lib:/usr/lib";
static char const default_pathext[]         = ".exe";


ksymhash_t
ksymhash_of(char const *__restrict text,
            size_t size) {
 // Taken from my other project TPP: 'tpp.c', '_TPPKeywordList_HashOfL'
 char const *end = text+size;
 ksymhash_t result = 1;
 kassertmem(text,size);
 for (; text != end; ++text) 
  result = result*263+*text;
 return result;
}

struct ksymbol *
ksymbol_new(char const *__restrict name) {
 struct ksymbol *result;
 size_t name_size = strlen(name);
 result = (struct ksymbol *)malloc(offsetof(struct ksymbol,s_name)+
                                  (name_size+1)*sizeof(char));
 if __likely(result) {
  result->s_hash = ksymhash_of(name,name_size);
  result->s_nmsz = name_size;
  memcpy(result->s_name,name,name_size*sizeof(char));
  result->s_name[name_size] = '\0';
 }
 return result;
}



void ksymtable_quit(struct ksymtable *self) {
 struct ksymtablebucket *biter,*bend;
 struct ksymbol *iter,*next;
 kassert_ksymtable(self);
 bend = (biter = self->st_mapv)+self->st_mapa;
 for (; biter != bend; ++biter) {
  if ((iter = biter->b_lname) != NULL) do {
   next = iter->s_nextname;
   free(iter);
  } while ((iter = next) != NULL);
 }
 free(self->st_mapv);
}


struct ksymbol const *
ksymtable_lookupname_h(struct ksymtable const *self, char const *symname,
                       size_t symnamelength, ksymhash_t hash) {
 struct ksymbol const *iter;
 kassert_ksymtable(self);
 if __unlikely(!self->st_mapa) return NULL;
 iter = self->st_mapv[ksymtable_modnamehash(self,hash)].b_lname;
 for (; iter; iter = iter->s_nextname) {
  if (iter->s_hash == hash &&
      iter->s_nmsz == symnamelength &&
      memcmp(iter->s_name,symname,symnamelength*sizeof(char)) == 0
      ) return iter; // Found it!
  
 }
 return NULL;
}

struct ksymbol const *
ksymtable_lookupaddr_single(struct ksymtable const *self,
                            ksymaddr_t addr) {
 struct ksymbol const *iter;
 kassert_ksymtable(self);
 if __unlikely(!self->st_mapa) return NULL;
 iter = self->st_mapv[ksymtable_modaddrhash(self,addr)].b_laddr;
 for (; iter; iter = iter->s_nextaddr) {
  if (iter->s_addr == addr) return iter; // Found it!
 }
 return NULL;
}


struct ksymbol const *
ksymtable_lookupaddr(struct ksymtable const *self,
                     ksymaddr_t addr, ksymaddr_t addr_min) {
 struct ksymbol const *result;
 while ((result = ksymtable_lookupaddr_single(self,addr)) == NULL &&
        addr-- != addr_min);
 return result;
}


kerrno_t
ksymtable_insert_inherited(struct ksymtable *__restrict self,
                           struct ksymbol *__restrict sym)
{
 size_t newsize;
 struct ksymtablebucket *bucket;
 kassert_ksymtable(self);
 kassertobj(sym);
 assert(ksymhash_of(sym->s_name,sym->s_nmsz) == sym->s_hash);
 assert((self->st_mapa != 0) == (self->st_mapv != NULL));
#if 0
 if (self->st_mapv) {
  newslot = self->st_mapv[ksymtable_modnamehash(self,sym->s_hash)].b_lname;
  while (newslot) {
   if (newslot->s_hash == sym->s_hash &&
       newslot->s_size == sym->s_size &&
       newslot->s_addr == sym->s_addr && /*< Should this really be in here? */
       memcmp(newslot->s_name,sym->s_name,sym->s_size) == 0
       ) return KS_UNCHANGED;
   newslot = newslot->s_nextname;
  }
 }
#endif
 newsize = self->st_size+1;
 if (self->st_mapa < newsize && KE_ISERR(ksymtable_rehash(self,newsize)) &&
    !self->st_mapa) return KE_NOMEM; // Failed to allocate the initial bucket
 assert(self->st_mapv);
 bucket = self->st_mapv+ksymtable_modnamehash(self,sym->s_hash);
 sym->s_nextname = bucket->b_lname;
 bucket->b_lname = sym;
 bucket = self->st_mapv+ksymtable_modaddrhash(self,sym->s_addr);
 sym->s_nextaddr = bucket->b_laddr;
 bucket->b_laddr = sym;
 assert(self->st_size == newsize-1);
 self->st_size = newsize;
 return KE_OK;
}


kerrno_t ksymtable_rehash(struct ksymtable *self,
                          size_t bucketcount) {
 struct ksymtablebucket *newvec;
 kassert_ksymtable(self);
 assert((self->st_mapa != 0) == (self->st_mapv != NULL));
 assertf(bucketcount != 0,"Invalid bucket count");
 // Allocate the new bucket vector (Use calloc to already initialize it to ZERO; aka. NULL)
 newvec = (struct ksymtablebucket *)calloc(bucketcount,
                                           sizeof(struct ksymtablebucket));
 if __unlikely(!newvec) return KE_NOMEM;
 if (self->st_mapa) {
  struct ksymtablebucket *biter,*bend,*dest;
  struct ksymbol *niter,*nnext;
  struct ksymbol *aiter,*anext;
  // The symbol table already has some entries (Must re-hash those)
  bend = (biter = self->st_mapv)+self->st_mapa;
  for (; biter != bend; ++biter) {
   if ((niter = biter->b_lname) != NULL) do {
    nnext = niter->s_nextname;
    // Insert the entry into its newly modulated bucket within the new bucket vector
    dest = newvec+ksymtable_modnamehash2(niter->s_hash,
                                         bucketcount);
    niter->s_nextname = dest->b_lname;
    dest->b_lname = niter;
   } while ((niter = nnext) != NULL);
   if ((aiter = biter->b_laddr) != NULL) do {
    anext = aiter->s_nextaddr;
    // Insert the entry into its newly modulated bucket within the new bucket vector
    dest = newvec+ksymtable_modaddrhash2(aiter->s_addr,
                                         bucketcount);
    aiter->s_nextaddr = dest->b_laddr;
    dest->b_laddr = aiter;
   } while ((aiter = anext) != NULL);
  }
  // Free the old bucket vector
  free(self->st_mapv);
 }
 self->st_mapv = newvec;
 self->st_mapa = bucketcount;
 return KE_OK;
}


kerrno_t
kshmregion_ploaddata(struct kshmregion *__restrict self,
                     uintptr_t offset, struct kfile *__restrict fp,
                     size_t filesz, pos_t pos) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = kfile_seek(fp,pos,SEEK_SET,NULL))) return error;
 return kshmregion_loaddata(self,offset,fp,filesz);
}
kerrno_t
kshmregion_loaddata(struct kshmregion *__restrict self,
                    uintptr_t offset, struct kfile *__restrict fp,
                    size_t filesz) {
 size_t maxbytes,copysize;
 __kernel void *addr; kerrno_t error;
 while (filesz) {
  addr = kshmregion_getphysaddr_s(self,offset,&maxbytes);
  copysize = min(maxbytes,filesz);
  offset += copysize,filesz -= copysize;
  error = kfile_kernel_readall(fp,addr,copysize);
  if __unlikely(KE_ISERR(error)) return error;
  if __unlikely(!filesz) break;
 }
 /* Fill the rest with ZEROs */
 while ((addr = kshmregion_getphysaddr_s(self,offset,&maxbytes)) != NULL) {
  memset(addr,0x00,maxbytes);
  offset += maxbytes;
 }
 return KE_OK;
}

void
ksecdata_cacheminmax(struct ksecdata *__restrict self) {
 struct kshlibsection *iter,*end;
 ksymaddr_t sec_min,sec_max,temp;
 kassertobj(self);
 end = (iter = self->ed_secv)+self->ed_secc;
 sec_min = (ksymaddr_t)-1,sec_max = 0;
 for (; iter != end; ++iter) {
  temp = iter->sls_base+iter->sls_size;
  if (iter->sls_albase < sec_min) sec_min = iter->sls_albase;
  if (temp > sec_max) sec_max = temp;
 }
 self->ed_begin = sec_min;
 self->ed_end = sec_max;
}

void ksecdata_quit(struct ksecdata *__restrict self) {
 struct kshlibsection *iter,*end;
 end = (iter = self->ed_secv)+self->ed_secc;
 for (; iter != end; ++iter) kshmregion_decref_full(iter->sls_region);
 free(self->ed_secv);
}


int 
ksecdata_ismapped(struct ksecdata const *__restrict self,
                  ksymaddr_t addr) {
 struct kshlibsection const *iter,*end;
 end = (iter = self->ed_secv)+self->ed_secc;
 for (; iter != end; ++iter) {
  if (addr >= iter->sls_base &&
      addr < iter->sls_base+iter->sls_size) return 1;
 }
 return 0;
}

__kernel void const *
ksecdata_translate_ro(struct ksecdata const *__restrict self,
                      ksymaddr_t addr, size_t *__restrict maxsize) {
 struct kshlibsection const *iter,*end;
 end = (iter = self->ed_secv)+self->ed_secc;
 for (; iter != end; ++iter) {
  if (addr >= iter->sls_base
  &&  addr < iter->sls_base+iter->sls_size
#if 0
  && (kshmregion_getflags(iter->sls_region)&KSHMREGION_FLAG_READ)
#endif
      ) {
   return kshmregion_getphysaddr_s(iter->sls_region,addr-iter->sls_albase,maxsize);
  }
 }
 return NULL;
}
__kernel void *
ksecdata_translate_rw(struct ksecdata *__restrict self,
                      ksymaddr_t addr, size_t *__restrict maxsize) {
 struct kshlibsection const *iter,*end;
 end = (iter = self->ed_secv)+self->ed_secc;
 for (; iter != end; ++iter) {
  if (addr >= iter->sls_base
  &&  addr < iter->sls_base+iter->sls_size
  && (kshmregion_getflags(iter->sls_region)&KSHMREGION_FLAG_WRITE)) {
   return kshmregion_getphysaddr_s(iter->sls_region,addr-iter->sls_albase,maxsize);
  }
 }
 return NULL;
}


size_t
ksecdata_getmem(struct ksecdata const *__restrict self, ksymaddr_t addr,
                void *__restrict buf, size_t bufsize) {
 void const *shptr; size_t maxbytes,copysize;
 kassertobj(self);
 while ((shptr = ksecdata_translate_ro(self,addr,&maxbytes)) != NULL) {
  copysize = min(maxbytes,bufsize);
  memcpy(buf,shptr,copysize);
  bufsize            -= copysize;
  if (copysize == bufsize) break;
  *(uintptr_t *)&buf += copysize;
 }
 return bufsize;
}
size_t
ksecdata_setmem(struct ksecdata *__restrict self, ksymaddr_t addr,
                void const *__restrict buf, size_t bufsize) {
 void *shptr; size_t maxbytes,copysize;
 kassertobj(self);
 while ((shptr = ksecdata_translate_rw(self,addr,&maxbytes)) != NULL) {
  copysize = min(maxbytes,bufsize);
  memcpy(shptr,buf,copysize);
  bufsize            -= copysize;
  if (copysize == bufsize) break;
  *(uintptr_t *)&buf += copysize;
 }
 return bufsize;
}

ksymaddr_t
kshlib_fileaddr2symaddr(struct kshlib const *__restrict self,
                        ksymaddr_t fileaddr) {
 struct kshlibsection const *iter,*end;
 end = (iter = self->sh_data.ed_secv)+self->sh_data.ed_secc;
 for (; iter != end; ++iter) {
  if (fileaddr >= iter->sls_filebase &&
      fileaddr < iter->sls_filebase+iter->sls_filesize
      ) return iter->sls_base+(fileaddr-iter->sls_filebase);
 }
 return KSYM_INVALID;
}
ksymaddr_t
kshlib_symaddr2fileaddr(struct kshlib const *__restrict self,
                        ksymaddr_t symaddr) {
 struct kshlibsection const *iter,*end;
 end = (iter = self->sh_data.ed_secv)+self->sh_data.ed_secc;
 for (; iter != end; ++iter) {
  if (symaddr >= iter->sls_base &&
      symaddr < iter->sls_base+iter->sls_filesize
      ) return iter->sls_filebase+(symaddr-iter->sls_base);
 }
 return KSYM_INVALID;
}



//////////////////////////////////////////////////////////////////////////
// === kreloc ===
__crit void kreloc_quit(struct kreloc *__restrict self) {
 struct krelocvec *iter,*end;
 KTASK_CRIT_MARK
 kassertobj(self);
 ksymlist_quit(&self->r_elf_syms);
 end = (iter = self->r_vecv)+self->r_vecc;
 for (; iter != end; ++iter) free(iter->rv_data);
 free(self->r_vecv);
}
__crit struct krelocvec *
kreloc_alloc(struct kreloc *__restrict self) {
 struct krelocvec *newvec;
 KTASK_CRIT_MARK
 kassertobj(self);
 assert((self->r_vecc == 0) == (self->r_vecv == NULL));
 newvec = (struct krelocvec *)realloc(self->r_vecv,(self->r_vecc+1)*
                                      sizeof(struct krelocvec));
 if __likely(newvec) {
  self->r_vecv = newvec;
  newvec += self->r_vecc++;
 }
 return newvec;
}




//////////////////////////////////////////////////////////////////////////
// === kshliblist ===
__crit void kshliblist_quit(struct kshliblist *__restrict self) {
 struct kshlib **iter,**end;
 KTASK_CRIT_MARK
 end = (iter = self->sl_libv)+self->sl_libc;
 for (; iter != end; ++iter) kshlib_decref(*iter);
 free(self->sl_libv);
}
__crit kerrno_t
kshliblist_append_inherited(struct kshliblist *__restrict self,
                            __ref struct kshlib *__restrict lib) {
 struct kshlib **newvec;
 KTASK_CRIT_MARK
 kassertobj(self);
 kassert_kshlib(lib);
 newvec = (struct kshlib **)realloc(self->sl_libv,(self->sl_libc+1)*
                                    sizeof(struct kshlib *));
 if __unlikely(!newvec) return KE_NOMEM;
 self->sl_libv = newvec;
 newvec[self->sl_libc++] = lib; /* Inherit reference. */
 return KE_OK;
}




void kshlib_destroy(struct kshlib *__restrict self) {
 kassert_object(self,KOBJECT_MAGIC_SHLIB);
 kshlibcache_dellib(self);
 kreloc_quit(&self->sh_reloc);
 ksecdata_quit(&self->sh_data);
 ksymtable_quit(&self->sh_privatesym);
 ksymtable_quit(&self->sh_weaksym);
 ksymtable_quit(&self->sh_publicsym);
 kshliblist_quit(&self->sh_deps);
 kfile_decref(self->sh_file);
 free(self);
}

__crit __ref struct ktask *
kshlib_spawn(struct kshlib const *__restrict self,
             struct kproc *__restrict proc) {
 struct ktask *result; void *useresp;
 KTASK_CRIT_MARK
 // Can't spawn main task in shared libraries
 if __unlikely(!(self->sh_flags&KMODFLAG_CANEXEC)) return NULL;
 result = ktask_newuser(ktask_self(),proc,&useresp,
                        KTASK_STACK_SIZE_DEF,KTASK_STACK_SIZE_DEF);
 if __unlikely(!result) return NULL;
 ktask_setupuser(result,useresp,(void *)self->sh_callbacks.slc_start);
 return result;
}


static struct kinode *
get_root_inode(struct kfile *__restrict __unused(self)) {
 return kdirent_getnode(kfs_getroot());
}
static struct kdirent *
get_root_dirent(struct kfile *__restrict __unused(self)) {
 struct kdirent *result;
 if __unlikely(KE_ISERR(kdirent_incref(result = kfs_getroot()))) result = NULL;
 return result;
}

struct kfiletype kshlibkernel_file_type = {
 .ft_size      = sizeof(struct kfile),
 .ft_getinode  = &get_root_inode,
 .ft_getdirent = &get_root_dirent,
};
struct kfile kshlibkernel_file = {
 KOBJECT_INIT(KOBJECT_MAGIC_FILE)
 0xffff,
 &kshlibkernel_file_type,
};

extern void _start(void);
struct kshlib __kshlib_kernel = {
 KOBJECT_INIT(KOBJECT_MAGIC_SHLIB)
 /* sh_refcnt                        */0xffff,
 /* sh_file                          */&kshlibkernel_file,
 /* sh_flags                         */KMODFLAG_FIXED|KMODFLAG_LOADED|KMODFLAG_CANEXEC|KMODKIND_KERNEL,
 /* sh_cidx                          */(size_t)-1,
 /* sh_publicsym                     */KSYMTABLE_INIT,
 /* sh_weaksym                       */KSYMTABLE_INIT,
 /* sh_privatesym                    */KSYMTABLE_INIT, /*< TODO: Add all kernel symbols to this for debugging information. */
 /* sh_deps                          */KSHLIBLIST_INIT,
 /* sh_data                          */{
 /* sh_data.ed_secc                  */0,
 /* sh_data.ed_secv                  */NULL},
 /* sh_reloc                         */{
 /* sh_reloc.r_vecc                  */0,
 /* sh_reloc.r_vecv                  */NULL,
 /* sh_reloc.r_elf_syms              */KSYMLIST_INIT},
 /* sh_callbacks                     */{
 /* sh_callbacks.slc_start           */(ksymaddr_t)(uintptr_t)&_start,
 /* sh_callbacks.slc_preinit_array_v */KSYM_INVALID,
 /* sh_callbacks.slc_preinit_array_c */0,
 /* sh_callbacks.slc_init            */KSYM_INVALID,
 /* sh_callbacks.slc_init_array_v    */KSYM_INVALID,
 /* sh_callbacks.slc_init_array_c    */0,
 /* sh_callbacks.slc_fini            */KSYM_INVALID,
 /* sh_callbacks.slc_fini_array_v    */KSYM_INVALID,
 /* sh_callbacks.slc_fini_array_c    */0}
};

__crit kerrno_t
kshlib_fopenfile(struct kfile *__restrict fp,
                 __ref struct kshlib **__restrict result) {
 KTASK_CRIT_MARK
 // Check for a cached version of the library
 if ((*result = kshlibcache_fgetlib(fp)) != NULL) return KE_OK;
 return kshlib_new(result,fp);
}


// Generate a sanitized true-root path with normalized
// slashes as well as prepending the true root path if
// the given pathenv isn't connected.
// Returns the required buffer size (Including the terminating ZERO-character).
// WARNING: This function is not perfect!
static size_t
get_true_path(char *buf, size_t bufsize,
              struct kfspathenv const *pathenv,
              char const *path, size_t pathmax) {
 size_t reqsize;
 char *end = buf+bufsize,*iter = buf;
 if (pathenv->env_root != kfs_getroot()) {
  /* Must prepend the path's root path. */
  kdirent_kernel_getpathname(pathenv->env_root,kfs_getroot(),iter,
                             iter < end ? (size_t)(end-iter) : 0,&reqsize);
  iter += (reqsize/sizeof(char))-1;
  /* 'kdirent_kernel_getpathname' doesn't include a trailing slash. - append it now! */
  if (iter < end) *iter = KFS_SEP;
  ++iter;
 }
 if (!pathmax || !KFS_ISSEP(path[0])) {
  /* The given path points into the CWD directory.
   * >> Now we must also prepend the given cwd path. */
  kdirent_kernel_getpathname(pathenv->env_cwd,pathenv->env_root,iter,
                             iter < end ? (size_t)(end-iter) : 0,&reqsize);
  iter += (reqsize/sizeof(char))-1;
  if (pathenv->env_cwd != kfs_getroot()) {
   /* 'kdirent_kernel_getpathname' doesn't include a trailing slash. - append it now! */
   if (iter < end) *iter = KFS_SEP;
   ++iter;
  }
 }
 /* Now must append the given path, and we're done. */
 /* WARNING: This isn't perfect because we don't
  *          handle '.' and '..' directory references. */
 while (pathmax && *path) {
  if (iter < end) {
   if ((*iter = *path) == KFS_ALTSEP) *iter = KFS_SEP;
  }
  ++iter;
  ++path;
  --pathmax;
 }
 if (iter < end) *iter = '\0';
 ++iter;
 return (size_t)(iter-buf);
}

static kerrno_t check_exec_permissions(struct kfile *__restrict fp) {
 mode_t mode; kerrno_t error;
 /* TODO: Check different bits based on filesystem UID/GID. */
 error = kfile_kernel_getattr(fp,KATTR_FS_PERM,&mode,sizeof(mode),NULL);
 if __unlikely(KE_ISERR(error)) return error;
 if (!(mode&(S_IXUSR|S_IXGRP|S_IXOTH)))
  return KE_NOEXEC; /* Don't allow execution without these bits. */
 return KE_OK;
}

__crit kerrno_t
kshlib_openfileenv(struct kfspathenv const *pathenv,
                   char const *__restrict filename, size_t filename_max,
                   __ref struct kshlib **__restrict result,
                   int require_exec_permissions) {
 struct kfile *fp; kerrno_t error;
 char trueroot_buf[PATH_MAX];
 char *trueroot,*newtrueroot;
 size_t trueroot_size,trueroot_reqsize;
 KTASK_CRIT_MARK
 /* Generate the ~true~ absolute path for the given filename. */
 trueroot = trueroot_buf,trueroot_size = sizeof(trueroot_buf);
again_true_root:
 trueroot_reqsize = get_true_path(trueroot,trueroot_size,
                                  pathenv,filename,filename_max);
 if (trueroot_reqsize > trueroot_size) {
  newtrueroot = (trueroot == trueroot_buf)
   ? (char *)malloc(trueroot_reqsize*sizeof(char))
   : (char *)realloc(trueroot,trueroot_reqsize*sizeof(char));
  if __unlikely(!newtrueroot) { error = KE_NOMEM; goto err_trueroot; }
  trueroot = newtrueroot;
  trueroot_size = trueroot_reqsize;
  goto again_true_root;
 }
 /* Check the SHLIB cache for the trueroot path. */
 if ((*result = kshlibcache_getlib(trueroot)) != NULL) {
  if (require_exec_permissions) {
   /* Make sure that the caller has execute permissions on the binary. */
   error = check_exec_permissions((*result)->sh_file);
   if __unlikely(KE_ISERR(error)) kshlib_decref(*result);
  } else {
   error = KE_OK;
  }
  goto err_trueroot;
 }
 /* Must opening a new shared library. */
 //printf("filename = %.*q\n",(unsigned)filename_max,filename);
 //printf("trueroot = %.*q\n",(unsigned)trueroot_size,trueroot);
 error = kdirent_openat(pathenv,trueroot,trueroot_size,O_RDONLY,0,NULL,&fp);
 if (KE_ISERR(error)) goto err_trueroot;
 if (require_exec_permissions) {
  /* If needed, make sure that the caller has execute permissions on the binary. */
  error = check_exec_permissions(fp);
  if __unlikely(KE_ISERR(error)) goto err_fptrueroot;
 }
 /* Since get_true_path isn't perfect, generate another path based
  * on the file we just opened, then use that to lookup the cache again!
  * NOTE: It wasn't perfect because it can't handle '.' and '..' references. */
again_true_root2:
 error = __kfile_kernel_getpathname_fromdirent(fp,kfs_getroot(),trueroot,
                                               trueroot_size,&trueroot_reqsize);
 /* NOTE: Not all files may be able to actually have a path associated
  *       with them. We still allow those files to be loaded if they
  *       are binaries, but we simply don't cache them. */
 if __likely(KE_ISOK(error)) {
  if (trueroot_reqsize > trueroot_size) {
   newtrueroot = (trueroot == trueroot_buf)
    ? (char *)malloc(trueroot_reqsize*sizeof(char))
    : (char *)realloc(trueroot,trueroot_reqsize*sizeof(char));
   if __unlikely(!newtrueroot) { error = KE_NOMEM; goto err_fptrueroot; }
   trueroot = newtrueroot;
   trueroot_size = trueroot_reqsize;
   goto again_true_root2;
  }
  /* Check the cache with the known true-root again. */
  if ((*result = kshlibcache_getlib(trueroot)) != NULL) {
   error = KE_OK;
   goto err_fptrueroot;
  }
  /* NOPE! This library definitely isn't cached, or the given path
   *       quite simply (while existing) isn't actually a valid binary. */
 }
 if (trueroot != trueroot_buf) free(trueroot);
 error = kshlib_new(result,fp);
 kfile_decref(fp);
 return error;
err_fptrueroot:
 kfile_decref(fp);
err_trueroot:
 if (trueroot != trueroot_buf) free(trueroot);
 return error;
}

__crit kerrno_t
kshlib_opensearch(struct kfspathenv const *pathenv,
                  char const *__restrict name, size_t namemax,
                  char const *__restrict search_paths, size_t search_paths_max,
                  __ref struct kshlib **__restrict result,
                  int require_exec_permissions) {
 char search_cat[PATH_MAX];
 char const *iter,*end,*next;
 kerrno_t error,last_error = KE_NOENT;
 KTASK_CRIT_MARK
 namemax = strnlen(name,namemax);
 end = (iter = search_paths)+search_paths_max;
 while (iter != end) {
  size_t partsize;
  assert(iter < end);
  next = strchr(iter,':');
  partsize = next ? next-iter : strlen(iter);
  if (partsize) {
   char *path; size_t reqsize;
   int hassep = KFS_ISSEP(iter[partsize-1]);
   if (!hassep) ++partsize;
   reqsize = (partsize+namemax+1)*sizeof(char);
   if (reqsize > sizeof(search_cat)) {
    path = (char *)malloc(reqsize);
    if __unlikely(!path) return KE_NOMEM;
   } else {
    path = search_cat;
   }
   memcpy(path,iter,partsize*sizeof(char));
   if (!hassep) path[partsize-1] = KFS_SEP;
   memcpy(path+partsize,name,namemax*sizeof(char));
   path[partsize+namemax] = '\0';
#if 0
   printf("Searchcat: %.*q + %.*q -> %.*q\n",
          (unsigned)(hassep ? partsize : partsize-1),iter,(unsigned)namemax,name,
          (unsigned)(partsize+namemax),path);
#endif
   error = kshlib_openfileenv(pathenv,path,(reqsize/sizeof(char))-1,
                              result,require_exec_permissions);
   if (path != search_cat) free(path);
   /* If we did manage to load the library, we've succeeded */
   if (KE_ISOK(error)) return error;
   /* Handle a hand full of errors specially */
   switch (error) {
    case KE_NOENT: break; /* File doesn't exist. */
    case KE_ACCES: last_error = error; break;
    case KE_DEVICE: return error; /*< A device error occurred. */
    case KE_NOEXEC:
    default:
     if (last_error == KE_NOENT) last_error = error;
     break;
   }
  }
  if (!next) break;
  iter = next+1;
 }
 return last_error;
}

static kerrno_t copyenvstring(char const *name, char **result, size_t *result_size) {
 char *resstring,*newresstring; kerrno_t error;
 size_t reqsize,ressize = 256*sizeof(char);
 struct kproc *caller = kproc_self();
 if __unlikely((resstring = (char *)malloc(ressize)) == NULL) return KE_NOMEM;
again_lookup:
 error = kproc_getenv(caller,name,(size_t)-1,resstring,ressize,&reqsize);
 if __unlikely(KE_ISERR(error)) {err_resstring: free(resstring); return error; }
 if (reqsize != ressize) {
  newresstring = (char *)realloc(resstring,reqsize*sizeof(char));
  if __unlikely(!newresstring) { error = KE_NOMEM; goto err_resstring; }
  resstring = newresstring;
  if (reqsize > ressize) { ressize = reqsize; goto again_lookup; }
 }
 *result = newresstring;
 *result_size = reqsize;
 return error;
}


__crit kerrno_t
kshlib_openlib(char const *__restrict name, size_t namemax,
               __ref struct kshlib **__restrict result) {
 kerrno_t error; struct kfspathenv env;
 env.env_root = NULL;
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  if (namemax && KFS_ISSEP(name[0])) {
   /* Absolute library path (open directly in respect to chroot-prisons). */
   error = kshlib_openfileenv(&env,name,namemax,result,0);
  } else {
   char *envval; size_t env_size;
   /* Check for process-local environment variable 'LD_LIBRARY_PATH'. */
   error = copyenvstring("LD_LIBRARY_PATH",&envval,&env_size);
   if (error == KE_NOENT) {
    error = kshlib_opensearch(&env,name,namemax,default_libsearch_paths,
                              __compiler_STRINGSIZE(default_libsearch_paths),
                              result,0);
   } else if (KE_ISERR(error)) {
    goto end_err;
   } else {
    error = kshlib_opensearch(&env,name,namemax,envval,env_size,result,0);
    free(envval);
   }
  }
 end_err:
  kfspathenv_quituser(&env);
  if __likely(KE_ISOK(error)) return error;
 }
 /* When searching for shared libraries, we somewhat break the rules
  * by allowing applications to search for libraries outside their
  * normal chroot-prison.
  * Hard restrictions are in-place though, and they are not allowed
  * to search in sub-folders or anywhere else for that matter because
  * we don't do so if the library name contains slashes.
  * >> By doing this, we make it much easier to place some executable
  *    in a chroot prison because you don't have to create a copy of
  *    all your libraries just for that one test. */
 {
  /* While questionable, this should still be completely save with respect
   * to how KOS granst rootfork() permissions, making it impossible to
   * use this mechanism to get the root-privileges of another library. */
  static char const fs_seps[2] = {KFS_SEP,KFS_ALTSEP};
  if (env.env_root != kfs_getroot() &&
     !strnpbrk(name,namemax,fs_seps,2)) {
   struct kfspathenv rootenv = KFSPATHENV_INITROOT;
   error = kshlib_opensearch(&rootenv,name,namemax,default_libsearch_paths,
                             __compiler_STRINGSIZE(default_libsearch_paths),
                             result,0);
  }
 }
 return error;
}



__crit kerrno_t
kshlib_openexe(char const *__restrict name, size_t namemax,
               __ref struct kshlib **__restrict result, __u32 flags) {
 kerrno_t error; struct kfspathenv env;
 char *env_path,*env_pathext;
 size_t env_path_size,env_pathext_size;
 if __unlikely(KE_ISERR(error = kfspathenv_inituser(&env))) return error;
 if (!(flags&KTASK_EXEC_FLAG_SEARCHPATH) || (namemax && KFS_ISSEP(name[0]))) {
  /* Absolute executable path (open directly in respect to chroot-prisons). */
  error = kshlib_openfileenv(&env,name,namemax,result,1);
 } else {
  /* Check for process-local environment variable 'PATH'. */
  error = copyenvstring("PATH",&env_path,&env_path_size);
  if __unlikely(KE_ISERR(error)) goto end_err;
  error = kshlib_opensearch(&env,name,namemax,env_path,env_path_size,result,1);
  if __unlikely(KE_ISERR(error)) {
   if (flags&KTASK_EXEC_FLAG_RESOLVEEXT) {
    char *partstart; size_t partsize;
    /* Re-scan by appending all parts of "$PATHEXT". */
    error = copyenvstring("PATHEXT",&env_pathext,&env_pathext_size);
    if __unlikely(KE_ISERR(error)) {
     if __unlikely(error != KE_NOENT) goto end_envpath;
     /* Default to using the hard-coded list of extensions. */
     env_pathext      = (char *)default_pathext;
     env_pathext_size = __compiler_STRINGSIZE(default_pathext);
    }
    namemax   = strnlen(name,namemax);
    partstart = env_pathext;
    while (env_pathext_size) {
     char *part_end = strnpbrk(partstart,env_pathext_size,":;",2);
     partsize = part_end ? part_end-partstart : env_pathext_size;
     assert(partsize <= env_pathext_size);
     if (partsize) {
      char *newname; size_t newnamesize;
      newnamesize = namemax+partsize;
      /* Concat the extension with the given path. */
      newname = (char *)malloc((newnamesize+1)*sizeof(char));
      if __unlikely(newname) { error = KE_NOMEM; goto end_envpathext; }
      memcpy(newname,name,namemax*sizeof(char));
      memcpy(newname+namemax,partstart,partsize*sizeof(char));
      newname[newnamesize] = '\0';
      /* Perform another scan, this time for the new name. */
      error = kshlib_opensearch(&env,newname,newnamesize,env_path,env_path_size,result,1);
      free(newname);
      if __likely(KE_ISOK(error)) break;
     }
     ++partsize;
     partstart        += partsize;
     env_pathext_size -= partsize;
     assert(partstart == part_end+1);
    }
end_envpathext:
    if (env_pathext != (char *)default_pathext) free(env_pathext);
   }
  }
end_envpath:
  free(env_path);
 }
end_err:
 kfspathenv_quituser(&env);
 return error;
}


__crit kerrno_t
kshlib_user_getinfo(struct kshlib *__restrict self,
                    __user void *module_base, kmodid_t id,
                    __user struct kmodinfo *buf, size_t bufsize,
                    __kernel size_t *reqsize, __u32 flags) {
 struct kmodinfo info;
 size_t info_size;
 kerrno_t error = KE_OK;
 KTASK_CRIT_MARK
 kassert_kshlib(self);
 kassertobjnull(reqsize);
 if ((flags&KMOD_INFO_FLAG_INFO) == KMOD_INFO_FLAG_INFO) {
  info.mi_id    = id;
  info.mi_kind  = self->sh_flags;
  info.mi_base  = module_base;
  info.mi_begin = (void *)((uintptr_t)module_base+self->sh_data.ed_begin);
  info.mi_end   = (void *)((uintptr_t)module_base+self->sh_data.ed_end);
  info.mi_start = (void *)((uintptr_t)module_base+self->sh_callbacks.slc_start);
 } else {
  memset(&info,0,offsetof(struct kmodinfo,mi_name));
  if (flags&KMOD_INFO_FLAG_ID) info.mi_id = id;
 }
 if (flags&KMOD_INFO_FLAG_NAME) {
  info_size    = min(bufsize,sizeof(struct kmodinfo));
  if (bufsize >= offsetafter(struct kmodinfo,mi_nmsz)) {
   if __unlikely(copy_from_user(&info.mi_name,&buf->mi_name,
                                sizeof(info.mi_name)+
                                sizeof(info.mi_nmsz))
                 ) return KE_FAULT;
   /* Check if the info-buffer should be re-used as name buffer. */
   if (!info.mi_name || !info.mi_nmsz) goto name_from_buf;
  } else {
name_from_buf:
   if (bufsize > sizeof(struct kmodinfo)) {
    info.mi_name = (char *)(buf+1);
    info.mi_nmsz = bufsize-sizeof(struct kmodinfo);
   } else {
    info.mi_name = NULL;
    info.mi_nmsz = 0;
   }
  }
  /* Query the module filename/filepath. */
  error = kfile_user_getattr(self->sh_file,
                            (flags&(KMOD_INFO_FLAG_PATH&~(KMOD_INFO_FLAG_NAME)))
                             ? KATTR_FS_PATHNAME : KATTR_FS_FILENAME,
                             info.mi_name,info.mi_nmsz,&info.mi_nmsz);
  if __unlikely(KE_ISERR(error)) return error;
  if (reqsize) *reqsize = sizeof(struct kmodinfo)+info.mi_nmsz;
 } else if ((flags&KMOD_INFO_FLAG_INFO) == KMOD_INFO_FLAG_INFO) {
  info_size = min(bufsize,offsetof(struct kmodinfo,mi_name));
  if (reqsize) *reqsize = offsetof(struct kmodinfo,mi_name);
 } else if (flags&KMOD_INFO_FLAG_ID) {
  info_size = min(bufsize,offsetafter(struct kmodinfo,mi_id));
  if (reqsize) *reqsize = offsetafter(struct kmodinfo,mi_id);
 } else {
  info_size = 0;
  if (reqsize) *reqsize = 0;
 }
 if __unlikely(copy_to_user(buf,&info,info_size)) error = KE_FAULT;
 return error;
}

static struct ksymbol const *
ksymtable_lookupaddr_size(struct ksymtable const *self,
                          ksymaddr_t sym_address, ksymaddr_t addr_min) {
 struct ksymbol const *result;
 result = ksymtable_lookupaddr(self,sym_address,addr_min);
 return (result && (!result->s_size || sym_address < result->s_addr+result->s_size)) ? result : NULL;
}

struct ksymbol const *
kshlib_get_closest_symbol(struct kshlib *__restrict self,
                          ksymaddr_t sym_address) {
 struct ksymbol const *symbols[3],*result;
 size_t diff,newdiff;
 kassert_kshlib(self);
 symbols[0] = ksymtable_lookupaddr_size(&self->sh_privatesym,sym_address,self->sh_data.ed_begin);
 symbols[1] = ksymtable_lookupaddr_size(&self->sh_publicsym,sym_address,self->sh_data.ed_begin);
 symbols[2] = ksymtable_lookupaddr_size(&self->sh_weaksym,sym_address,self->sh_data.ed_begin);
              result = symbols[0];
 if (!result) result = symbols[1];
 if (!result) result = symbols[2];
 if (result) {
  diff = sym_address-result->s_addr;
#define BETTER(x) \
  if (x) { newdiff = sym_address-(x)->s_addr; \
  if (newdiff < diff) { diff = newdiff; result = (x); } }
  BETTER(symbols[1]);
  BETTER(symbols[2]);
#undef BETTER
 }
 return result;
}

struct ksymbol const *
kshlib_get_any_symbol(struct kshlib *__restrict self,
                      char const *name, __size_t name_size,
                      __size_t name_hash) {
 struct ksymbol const *result;
 kassert_kshlib(self);
              result = ksymtable_lookupname_h(&self->sh_publicsym,name,name_size,name_hash);
 if (!result) result = ksymtable_lookupname_h(&self->sh_privatesym,name,name_size,name_hash);
 if (!result) result = ksymtable_lookupname_h(&self->sh_weaksym,name,name_size,name_hash);
 return result;
}




__DECL_END

#ifndef __INTELLISENSE__
#include "linker-cache.c.inl"
#include "linker-elf32.c.inl"
#include "linker-pe32.c.inl"
#include "linker-recent.c.inl"
#include "linker-relocate.c.inl"
#include "linker-slloaders.c.inl"
#include "procmodules.c.inl"
#endif

#endif /* !__KOS_KERNEL_LINKER_C__ */
