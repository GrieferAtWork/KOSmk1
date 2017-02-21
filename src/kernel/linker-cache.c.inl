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
#ifndef __KOS_KERNEL_LINKER_CACHE_C_INL__
#define __KOS_KERNEL_LINKER_CACHE_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/types.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/fs/file.h>
#include <sys/types.h>
#include <malloc.h>
#include <unistd.h>
#include <stddef.h>
#include <kos/syslog.h>
#include <kos/kernel/fs/fs.h>

__DECL_BEGIN

#define kshlibpath_hashof  ksymhash_of

struct kshlibcachetab {
 struct kshlibcachetab *ct_prev;    /*< [0..1] Previous tab with the same modulated name hash. */
 struct kshlibcachetab *ct_next;    /*< [0..1][owned] Next tab with the same modulated name hash. */
 struct kshlib         *ct_lib;     /*< [1..1] Library cached within this tab. */
 size_t                 ct_hash;    /*< Hash of the path name. */
 size_t                 ct_size;    /*< Length of the path name. */
 char                   ct_name[1]; /*< [ct_size] Inlined absolute path name of the library. */
};

struct kshlibcachebucket {
 struct kshlibcachetab *cb_tabs; /*< [0..1] Linked list of tabs sharing the modulated hash of this bucket. */
 struct kshlibcachetab *cb_lib;  /*< [0..1] Cached library with bucket's cache index (sh_cidx). */
};

struct kshlibcache {
 size_t                    c_freelib; /*< Hint towards a bucket with a free 'cb_lib' slot. */
 size_t                    c_bucketc; /*< Amount of buckets in use. */
 size_t                    c_bucketa; /*< Allocated amount of buckets. */
 struct kshlibcachebucket *c_bucketv; /*< [0..1][owned][0..c_bucketa][owned] Vector of buckets. */
};

// Global cache of known shared libraries
static struct kshlibcache kslcache = {0,0,0,NULL};
static struct kmutex kslcache_lock = KMUTEX_INIT;

__local kerrno_t kshlibcache_rehash(size_t new_bucket_count) {
 struct kshlibcachebucket *newvec,*biter,*bend,*dest;
 struct kshlibcachetab *titer,*tnext;
 assert(new_bucket_count);
 assert(new_bucket_count >= kslcache.c_bucketc);
 newvec = (struct kshlibcachebucket *)calloc(new_bucket_count,
                                             sizeof(struct kshlibcachebucket));
 if __unlikely(!newvec) return KE_NOMEM;
 bend = (biter = kslcache.c_bucketv)+kslcache.c_bucketa;
 dest = newvec;
 /* Update cache ids of all loaded libraries. */
 for (; biter != bend; ++biter) if (biter->cb_lib) {
  dest->cb_lib = biter->cb_lib;
  kassert_kshlib(dest->cb_lib->ct_lib);
  dest->cb_lib->ct_lib->sh_cidx = (size_t)(dest-newvec);
  ++dest;
 }
 assert(dest <= newvec+new_bucket_count);
 kslcache.c_freelib = (size_t)(dest-newvec);
 k_syslogf(KLOG_DEBUG,"[SHLIB] Rehasing cache to %Iu entries\n",new_bucket_count);

 biter = kslcache.c_bucketv;
 for (; biter != bend; ++biter) {
  titer = biter->cb_tabs;
  /* Transfer (and rehash) all paths within this bucket */
  while (titer) {
   tnext = titer->ct_next;
   dest = &newvec[titer->ct_hash % new_bucket_count];
   titer->ct_prev = NULL;
   if ((titer->ct_next = dest->cb_tabs) != NULL)
    titer->ct_next->ct_prev = titer;
   dest->cb_tabs = titer;
   titer = tnext;
  }
 }
 free(kslcache.c_bucketv);
 kslcache.c_bucketv = newvec;
 kslcache.c_bucketa = new_bucket_count;
 return KE_OK;
}

__local kerrno_t kshlibcache_instab(struct kshlibcachetab *tab) {
 struct kshlibcachebucket *bucket; kerrno_t error;
 if __unlikely(KE_ISERR(error = kmutex_lock(&kslcache_lock))) return error;
 assert(kslcache.c_bucketc <= kslcache.c_bucketa);
 if (kslcache.c_bucketc == kslcache.c_bucketa) {
  // Must allocate more buckets
  error = kshlibcache_rehash(kslcache.c_bucketa ? kslcache.c_bucketa*2 : 2);
  if __unlikely(KE_ISERR(error)) goto end;
 }
 bucket = &kslcache.c_bucketv[tab->ct_hash % kslcache.c_bucketa];
 tab->ct_prev = NULL;
 if ((tab->ct_next = bucket->cb_tabs) != NULL)
  tab->ct_next->ct_prev = tab;
 bucket->cb_tabs = tab;
 if (kslcache.c_freelib < kslcache.c_bucketa) {
  bucket = kslcache.c_bucketv+kslcache.c_freelib++;
 } else {
  bucket = kslcache.c_bucketv;
 }
 // Search for a free lib slot
 // NOTE: This should never result in an infinite loop,
 //       as we had just made sure that there are at least
 //       as many buckets are there are slots.
 while (bucket->cb_lib) {
  if (++bucket == kslcache.c_bucketv+kslcache.c_bucketa)
   bucket = kslcache.c_bucketv;
 }
 // Store the bucket index as cache index
 bucket->cb_lib = tab;
 tab->ct_lib->sh_cidx = (size_t)(bucket-kslcache.c_bucketv);
 ++kslcache.c_bucketc;
end:
 kmutex_unlock(&kslcache_lock);
 return error;
}


#define kshlibcachetab_delete  free
__local struct kshlibcachetab *
kshlibcachetab_new(struct kshlib *lib) {
 struct kshlibcachetab *result,*newresult;
 size_t reqsize,newreqsize;
 kerrno_t error;
 // First attempt: Assume filename not longer than 'PATH_MAX'
 result = (struct kshlibcachetab *)malloc(offsetafter(struct kshlibcachetab,ct_size)+
                                         (PATH_MAX+1)*sizeof(char));
 error = __kfile_kernel_getpathname_fromdirent(lib->sh_file,kfs_getroot(),
                                        result ? result->ct_name : NULL,
                                        result ? (PATH_MAX+1)*sizeof(char) : 0,
                                        &reqsize);
 if __unlikely(KE_ISERR(error)) { free(result); return NULL; }
 if (reqsize != (PATH_MAX+1)*sizeof(char)) {
  newreqsize = (PATH_MAX+1)*sizeof(char);
again_getpath:
  newresult = (struct kshlibcachetab *)realloc(result,offsetafter(struct kshlibcachetab,ct_size)+
                                               reqsize);
  if (!newresult) { free(result); return NULL; }
  result = newresult;
  if (reqsize > newreqsize) {
   error = __kfile_kernel_getpathname_fromdirent(lib->sh_file,kfs_getroot(),
                                          result->ct_name,reqsize,
                                          &newreqsize);
   if __unlikely(KE_ISERR(error)) { free(result); return NULL; }
   if __unlikely(newreqsize != reqsize) { reqsize = newreqsize; goto again_getpath; }
  }
 } else if (!result) return NULL;
 result->ct_size = (reqsize/sizeof(char))-1;
 // The filename, and its size has been retrieved (in sanitized form)
 result->ct_hash = kshlibpath_hashof(result->ct_name,result->ct_size);
 result->ct_lib = lib;
 // We're not responsible for initializing 'ct_prev' or 'ct_next'
 return result;
}


__crit __wunused kerrno_t
kshlibcache_addlib(struct kshlib *__restrict lib) {
 struct kshlibcachetab *tab; kerrno_t error;
 KTASK_CRIT_MARK
 if __unlikely((tab = kshlibcachetab_new(lib)) == NULL) return KE_NOMEM;
 error = kshlibcache_instab(tab);
 if __unlikely(KE_ISERR(error)) kshlibcachetab_delete(tab);
 return error;
}

__crit void kshlibcache_dellib(struct kshlib *__restrict lib) {
 struct kshlibcachebucket *bucket;
 struct kshlibcachetab *tab;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kmutex_lock(&kslcache_lock))) {
  assert(!kslcache.c_bucketc);
  assert(!kslcache.c_bucketa);
  assert(!kslcache.c_bucketv);
  return;
 }
 if (lib->sh_cidx >= kslcache.c_bucketa) goto end;
 bucket = &kslcache.c_bucketv[lib->sh_cidx];
 tab = bucket->cb_lib;
 if (!tab || tab->ct_lib != lib) goto end;
 assertf((tab->ct_prev == NULL) == (tab == kslcache.c_bucketv[tab->ct_hash % kslcache.c_bucketa].cb_tabs)
        ,"tab->ct_prev                                                  = %p\n"
         "tab                                                           = %p\n"
         "kslcache.c_bucketv[tab->ct_hash %% kslcache.c_bucketa].cb_tabs = %p\n"
        ,tab->ct_prev,tab
        ,kslcache.c_bucketv[tab->ct_hash % kslcache.c_bucketa].cb_tabs);
 if (tab->ct_next) tab->ct_next->ct_prev = tab->ct_prev;
 if (tab->ct_prev) tab->ct_prev->ct_next = tab->ct_next;
 else kslcache.c_bucketv[tab->ct_hash % kslcache.c_bucketa].cb_tabs = tab->ct_next;
 kslcache.c_freelib = lib->sh_cidx;
 bucket->cb_lib = NULL;
 kshlibcachetab_delete(tab);
 if (!--kslcache.c_bucketc) {
  free(kslcache.c_bucketv);
  kslcache.c_bucketa = 0;
  kslcache.c_bucketv = NULL;
 } else if (kslcache.c_bucketc <= kslcache.c_bucketa/2 &&
            lib->sh_cidx == kslcache.c_bucketa-1) {
  // Reduce allocated size
  size_t newalloc = kslcache.c_bucketa/2;
  assert(newalloc);
  kshlibcache_rehash(newalloc);
 }
end:
 kmutex_unlock(&kslcache_lock);
}


__crit __ref struct kshlib *
kshlibcache_fgetlib(struct kfile *fp) {
 __ref struct kshlib *result; char *filename;
 KTASK_CRIT_MARK
 filename = kfile_getmallname(fp);
 if __unlikely(!filename) return NULL;
 result = kshlibcache_getlib(filename);
 free(filename);
 return result;
}

__crit __ref struct kshlib *
kshlibcache_getlib(char const *__restrict absolute_path) {
 __ref struct kshlib *result;
 struct kshlibcachetab *tabs;
 size_t path_size,hash;
 KTASK_CRIT_MARK
 path_size = strlen(absolute_path);
 hash = kshlibpath_hashof(absolute_path,path_size);
 if __unlikely(KE_ISERR(kmutex_lock(&kslcache_lock))) return NULL;
 if __unlikely(!kslcache.c_bucketa) result = NULL;
 else {
  tabs = kslcache.c_bucketv[hash % kslcache.c_bucketa].cb_tabs;
  while (tabs) {
   kassert_kshlib(tabs->ct_lib);
   assertf(tabs->ct_lib->sh_cidx < kslcache.c_bucketa,
           "Tab library not mapped correctly (out-of-bounds cache index)");
   assertf(tabs == kslcache.c_bucketv[tabs->ct_lib->sh_cidx].cb_lib,
           "Tab library not mapped correctly (miss-matching cache reference)");
   if (tabs->ct_hash == hash &&
       tabs->ct_size == path_size &&
       memcmp(tabs->ct_name,absolute_path,path_size) == 0) {
    result = tabs->ct_lib; // Found it
    if __unlikely(KE_ISERR(kshlib_incref(result))) result = NULL;
    goto end;
   }
   tabs = tabs->ct_next;
  }
  result = NULL;
 }
end:
 kmutex_unlock(&kslcache_lock);
 return result;
}


__DECL_END

#endif /* !__KOS_KERNEL_LINKER_CACHE_C_INL__ */
