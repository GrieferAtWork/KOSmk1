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
#ifdef __INTELLISENSE__
#include "fat-internal-enum.c.inl"
__DECL_BEGIN
#define DIRISSEC
#endif

#ifndef __longnamebufferentry_defined
#define __longnamebufferentry_defined
struct longnamebufferentry {
 __u8 ne_namepos;
 char ne_namedata[13];
};
struct longnamebuffer {
 size_t                      ln_entryc; /*< Amount of long name entries. */
 struct longnamebufferentry *ln_entryv; /*< [0..ln_entryc][owned] Vector of long name entries (sorted by 'ne_namepos'). */
};
#define longnamebuffer_init(self)  (void)((self)->ln_entryc = 0,(self)->ln_entryv = NULL)
#define longnamebuffer_quit(self)  free((self)->ln_entryv)
#define longnamebuffer_clear(self) (longnamebuffer_quit(self),longnamebuffer_init(self))

__local kerrno_t longnamebuffer_insertfile(struct longnamebuffer *self, struct kfatfileheader *file) {
 struct longnamebufferentry *newvec,*inspos,*insend; __u8 prio; char *namedata;
 newvec = (struct longnamebufferentry *)realloc(self->ln_entryv,(self->ln_entryc+1)*
                                                sizeof(struct longnamebufferentry));
 if __unlikely(!newvec) return KE_NOMEM;
 self->ln_entryv = newvec;
 insend = (inspos = newvec)+(self->ln_entryc++);
 prio = file->f_marker;
 // Search for where we need to insert this long filename entry
 while (inspos != insend) {
  if (inspos->ne_namepos > prio) {
   memmove(inspos+1,inspos,(size_t)(insend-inspos)*
           sizeof(struct longnamebufferentry));
   break;
  }
  ++inspos;
 }
 inspos->ne_namepos = prio;
 namedata = (char *)file;
 // Fill the name data of the long filename
#define charat(base,i) namedata[base+i*2]
 inspos->ne_namedata[0]  = charat(1,0);
 inspos->ne_namedata[1]  = charat(1,1);
 inspos->ne_namedata[2]  = charat(1,2);
 inspos->ne_namedata[3]  = charat(1,3);
 inspos->ne_namedata[4]  = charat(1,4);
 inspos->ne_namedata[5]  = charat(14,0);
 inspos->ne_namedata[6]  = charat(14,1);
 inspos->ne_namedata[7]  = charat(14,2);
 inspos->ne_namedata[8]  = charat(14,3);
 inspos->ne_namedata[9]  = charat(14,4);
 inspos->ne_namedata[10] = charat(14,5);
 inspos->ne_namedata[11] = charat(28,0);
 inspos->ne_namedata[12] = charat(28,1);
#undef charat
 return KE_OK;
}
#endif



#ifdef DIRISSEC
kerrno_t kfatfs_enumdirsec(struct kfatfs *self, kfatsec_t dirsec, __u32 maxsize,
                           pkfatfs_enumdir_callback callback, void *closure)
#else
kerrno_t kfatfs_enumdir(struct kfatfs *self, kfatcls_t dir,
                        pkfatfs_enumdir_callback callback, void *closure)
#endif
{
 struct kfatfileheader *files,*iter,*end;
#ifdef DIRISSEC
 struct kfatfileheader *maxend;
#endif
 kerrno_t error; struct longnamebuffer longfilename;
 kassertobj(self);
 kassertbyte(callback);
 longnamebuffer_init(&longfilename);
#ifndef DIRISSEC
 if __unlikely(kfatfs_iseofcluster(self,dir)) return KE_OK;
#endif
#ifdef DIRISSEC
 files = (struct kfatfileheader *)malloc(self->f_secsize);
 if __unlikely(!files) return KE_NOMEM;
 if ((maxend = files+maxsize) < files)
  maxend = (struct kfatfileheader *)(uintptr_t)-1;
 for (; files < maxend; *(uintptr_t *)&maxend -= self->f_secsize,++dirsec)
#else
 files = (struct kfatfileheader *)malloc(self->f_secsize*self->f_sec4clus);
 if __unlikely(!files) return KE_NOMEM;
 for (;;)
#endif
 {
  struct kfatfilepos fpos;
#ifdef DIRISSEC
  end = (iter = files)+(self->f_secsize/sizeof(struct kfatfileheader));
  if (maxend < end) end = maxend;
  error = kfatfs_loadsectors(self,dirsec,1,files);
#else
  kfatsec_t dirsec;
  end = (iter = files)+((self->f_secsize*self->f_sec4clus)/sizeof(struct kfatfileheader));
  dirsec = kfatfs_clusterstart(self,dir);
  error = kfatfs_loadsectors(self,dirsec,self->f_sec4clus,files);
#endif
  if __unlikely(KE_ISERR(error)) goto end;
  fpos.fm_namepos = (__u64)(dirsec*self->f_secsize);
  for (; iter < end; ++iter) {
   if (iter->f_marker == KFATFILE_MARKER_DIREND) break;
   if (iter->f_marker == KFATFILE_MARKER_UNUSED) continue;
   if (iter->f_attr == KFATFILE_ATTR_LONGFILENAME) {
    if __unlikely(KE_ISERR(error = longnamebuffer_insertfile(&longfilename,iter))) goto end;
    continue;
   }
   fpos.fm_headpos = (__u64)(dirsec*self->f_secsize)+((uintptr_t)iter-(uintptr_t)files);
   if (longfilename.ln_entryc) {
    struct longnamebufferentry *entryiter,*entryend;
    size_t longfilenamesize;
    char *longname = (char *)longfilename.ln_entryv;
    entryend = (entryiter = longfilename.ln_entryv)+longfilename.ln_entryc;
    while (entryiter != entryend) {
     memmove(longname,entryiter->ne_namedata,13*sizeof(char));
     ++entryiter;
     longname += 13;
    }
    longfilenamesize = (size_t)(longname-(char *)longfilename.ln_entryv);
    longname = (char *)longfilename.ln_entryv;
    while (longfilenamesize && (longname[longfilenamesize-1] == '\xff')) --longfilenamesize;
    if __likely(longfilenamesize) {
     if (longname[longfilenamesize-1] != '\0') longname[longfilenamesize] = '\0';
     else while (longfilenamesize && (longname[longfilenamesize-1] == '\0')) --longfilenamesize;
    }
    error = (*callback)(self,&fpos,iter,longname,longfilenamesize,closure);
    longnamebuffer_clear(&longfilename);
    fpos.fm_namepos = (__u64)(dirsec*self->f_secsize)+((uintptr_t)iter-(uintptr_t)files);
   } else {
    char filename[13]; char *filenameiter;
    // Short filename
    filenameiter = filename;
    // I don't Fu36ing care for special directory entries.
    // >> KOS doesn't use them, and when if comes to user-level
    //    APIS that require them, they're simply being emulated!
    if (iter->f_name[0] == '.' && (iter->f_name[1] == ' ' ||
       (iter->f_name[1] == '.' && iter->f_name[2] == ' '))) continue;
    memcpy(filenameiter,iter->f_name,8*sizeof(char)),filenameiter += 8;
    while (filenameiter != filename && isspace(filenameiter[-1])) --filenameiter;
    if (iter->f_ntflags&KFATFILE_NFLAG_LOWBASE) {
     char *tempiter;
     for (tempiter = filename; tempiter != filenameiter;
          ++tempiter) *tempiter = tolower(*tempiter);
    }
    *filenameiter++ = '.';
    memcpy(filenameiter,iter->f_ext,3*sizeof(char)),filenameiter += 3;
    while (filenameiter != filename && isspace(filenameiter[-1])) --filenameiter;
    if (iter->f_ntflags&KFATFILE_NFLAG_LOWEXT) {
     char *tempiter = filenameiter;
     while (tempiter[-1] != '.') { --tempiter; *tempiter = tolower(*tempiter); }
    }
    if (filenameiter != filename && filenameiter[-1] == '.') --filenameiter;
    *filenameiter = '\0';
    if (*(unsigned char *)filename == KFATFILE_MARKER_IS0XE5) *(unsigned char *)filename = 0xE5;
    fpos.fm_namepos = (__u64)(dirsec*self->f_secsize)+((uintptr_t)iter-(uintptr_t)files);
    error = (*callback)(self,&fpos,iter,filename,(size_t)(filenameiter-filename),closure);
   }
   if __unlikely(error != KE_OK) goto end_allways;
  }
  if (iter != end) goto end;
#ifndef DIRISSEC
  // Read the next cluster
  // >> Used for non-root FAT12/16, or any FAT32 directory
  error = kfatfs_fat_read(self,dir,&dir);
  if __unlikely(KE_ISERR(error)) break;
  if (kfatfs_iseofcluster(self,dir)) break;
#else
  // Go to the next sector
  // >> Used for FAT12/16 root directories
  ++dirsec;
#endif
 }
end:
 longnamebuffer_quit(&longfilename);
end_allways:
 free(files);
 return error;
}


#ifdef DIRISSEC
#undef DIRISSEC
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif
