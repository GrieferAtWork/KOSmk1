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
#ifndef __KOS_KERNEL_FS_VFS_DEV_C__
#define __KOS_KERNEL_FS_VFS_DEV_C__ 1

#include <kos/kernel/addist.h>
#include <kos/kernel/fd.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/fs/fs-dirfile.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/vfs-dev.h>
#include <kos/kernel/fs/vfs.h>
#include <kos/kernel/keyboard.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/tty.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// === Keyboard ===
static void vfsfile_dev_kb_quit(struct kfile *__restrict self) {
 kaddist_deluser(&keyboard_input,self);
 kvfile_quit(self);
}
static kerrno_t
vfsfile_dev_kb_open(struct kfile *__restrict self, struct kdirent *__restrict dirent,
                    struct kinode *__restrict inode, __openmode_t mode) {
 kerrno_t error = kvfile_open(self,dirent,inode,mode);
 if __likely(KE_ISOK(error)) {
  error = kaddist_adduser(&keyboard_input,self);
  if __unlikely(KE_ISERR(error)) kvfile_quit(self);
 }
 return error;
}


static kerrno_t
vfsfile_dev_kbevent_read(struct kfile *__restrict self, void *__restrict buf,
                         size_t bufsize, size_t *__restrict rsize) {
 if (bufsize < sizeof(struct kbevent)) { *rsize = 0; return KE_OK; }
 *rsize = sizeof(struct kbevent);
 return kaddist_vrecv(&keyboard_input,buf,self);
}
static kerrno_t
vfsfile_dev_kbkey_read(struct kfile *__restrict self, void *__restrict buf,
                       size_t bufsize, size_t *__restrict rsize) {
 struct kbevent event; kerrno_t error;
 if (bufsize < sizeof(kbkey_t)) { *rsize = 0; return KE_OK; }
 error = kaddist_vrecv(&keyboard_input,&event,self);
 if __likely(KE_ISOK(error)) {
  *(kbkey_t *)buf = event.e_key;
  *rsize = sizeof(kbkey_t);
 }
 return error;
}
static kerrno_t
vfsfile_dev_kbscan_read(struct kfile *__restrict self, void *__restrict buf,
                          size_t bufsize, size_t *__restrict rsize) {
 struct kbevent event; kerrno_t error;
 if (bufsize < sizeof(kbscan_t)) { *rsize = 0; return KE_OK; }
 error = kaddist_vrecv(&keyboard_input,&event,self);
 if __likely(KE_ISOK(error)) {
  *(kbscan_t *)buf = event.e_scan;
  *rsize = sizeof(kbscan_t);
 }
 return error;
}
static kerrno_t
vfsfile_dev_kbtext_read(struct kfile *__restrict self, void *__restrict buf,
                        size_t bufsize, size_t *__restrict rsize) {
 struct kbevent event; kerrno_t error;
 if (bufsize < sizeof(char)) { *rsize = 0; return KE_OK; }
 *rsize = sizeof(char);
 do {
  error = kaddist_vrecv(&keyboard_input,&event,self);
  if __unlikely(KE_ISERR(error)) return error;
 } while (!KEY_ISUTF8(event.e_key) || !KEY_ISDOWN(event.e_key));
 *(char *)buf = KEY_TOUTF8(event.e_key);
 return error;
}

struct kfiletype kvfiletype_dev_kbevent = {
 .ft_size      = sizeof(struct kvfile),
 .ft_read      = &vfsfile_dev_kbevent_read,
 .ft_quit      = &vfsfile_dev_kb_quit,
 .ft_open      = &vfsfile_dev_kb_open,
 .ft_getinode  = &kvfile_getinode,
 .ft_getdirent = &kvfile_getdirent,
};
struct kfiletype kvfiletype_dev_kbkey = {
 .ft_size      = sizeof(struct kvfile),
 .ft_read      = &vfsfile_dev_kbkey_read,
 .ft_quit      = &vfsfile_dev_kb_quit,
 .ft_open      = &vfsfile_dev_kb_open,
 .ft_getinode  = &kvfile_getinode,
 .ft_getdirent = &kvfile_getdirent,
};
struct kfiletype kvfiletype_dev_kbscan = {
 .ft_size      = sizeof(struct kvfile),
 .ft_read      = &vfsfile_dev_kbscan_read,
 .ft_quit      = &vfsfile_dev_kb_quit,
 .ft_open      = &vfsfile_dev_kb_open,
 .ft_getinode  = &kvfile_getinode,
 .ft_getdirent = &kvfile_getdirent,
};
struct kfiletype kvfiletype_dev_kbtext = {
 .ft_size      = sizeof(struct kvfile),
 .ft_read      = &vfsfile_dev_kbtext_read,
 .ft_quit      = &vfsfile_dev_kb_quit,
 .ft_open      = &vfsfile_dev_kb_open,
 .ft_getinode  = &kvfile_getinode,
 .ft_getdirent = &kvfile_getdirent,
};



//////////////////////////////////////////////////////////////////////////
// === Special files ===
static kerrno_t vfsfile_dev_full_write(struct kfile *__restrict __unused(self), void const *__restrict __unused(buf), size_t __unused(bufsize), size_t *__restrict wsize) { *wsize = 0; return KE_OK; }
static kerrno_t vfsfile_dev_full_pwrite(struct kfile *__restrict __unused(self), pos_t __unused(pos), void const *__restrict __unused(buf), size_t __unused(bufsize), size_t *__restrict wsize) { *wsize = 0; return KE_OK; }
static kerrno_t vfsfile_dev_zero_read(struct kfile *__restrict __unused(self), void *__restrict buf, size_t bufsize, size_t *__restrict rsize) { memset(buf,0,*rsize = bufsize); return KE_OK; }
static kerrno_t vfsfile_dev_zero_pread(struct kfile *__restrict __unused(self), pos_t __unused(pos), void *__restrict buf, size_t bufsize, size_t *__restrict rsize) { memset(buf,0,*rsize = bufsize); return KE_OK; }
static kerrno_t vfsfile_dev_null_read(struct kfile *__restrict __unused(self), void *__restrict __unused(buf), size_t __unused(bufsize), size_t *__restrict rsize) { *rsize = 0; return KE_OK; }
static kerrno_t vfsfile_dev_null_pread(struct kfile *__restrict __unused(self), pos_t __unused(pos), void *__restrict __unused(buf), size_t __unused(bufsize), size_t *__restrict rsize) { *rsize = 0; return KE_OK; }
static kerrno_t vfsfile_dev_null_write(struct kfile *__restrict __unused(self), void const *__restrict __unused(buf), size_t bufsize, size_t *__restrict wsize) { *wsize = bufsize; return KE_OK; }
static kerrno_t vfsfile_dev_null_pwrite(struct kfile *__restrict __unused(self), pos_t __unused(pos), void const *__restrict __unused(buf), size_t bufsize, size_t *__restrict wsize) { *wsize = bufsize; return KE_OK; }
static kerrno_t vfsfile_dev_null_seek(struct kfile *__restrict __unused(self), off_t __unused(off), int __unused(whence), pos_t *__restrict newpos) { *newpos = 0; return KE_OK; }
static kerrno_t vfsfile_dev_null_trunc(struct kfile *__restrict __unused(self), pos_t __unused(newsize)) { return KE_OK; }
static kerrno_t vfsfile_dev_null_flush(struct kfile *__restrict __unused(self)) { return KE_OK; }

struct kfiletype kvfiletype_dev_full = {
 KVFILE_TYPE_INIT
 .ft_size   = sizeof(struct kvfile),
 .ft_read   = &vfsfile_dev_zero_read,
 .ft_write  = &vfsfile_dev_full_write,
 .ft_pread  = &vfsfile_dev_zero_pread,
 .ft_pwrite = &vfsfile_dev_full_pwrite,
 .ft_seek   = &vfsfile_dev_null_seek,
 .ft_trunc  = &vfsfile_dev_null_trunc,
 .ft_flush  = &vfsfile_dev_null_flush,
};
struct kfiletype kvfiletype_dev_zero = {
 KVFILE_TYPE_INIT
 .ft_size   = sizeof(struct kvfile),
 .ft_read   = &vfsfile_dev_zero_read,
 .ft_write  = &vfsfile_dev_null_write,
 .ft_pread  = &vfsfile_dev_zero_pread,
 .ft_pwrite = &vfsfile_dev_null_pwrite,
 .ft_seek   = &vfsfile_dev_null_seek,
 .ft_trunc  = &vfsfile_dev_null_trunc,
 .ft_flush  = &vfsfile_dev_null_flush,
};
struct kfiletype kvfiletype_dev_null = {
 KVFILE_TYPE_INIT
 .ft_size   = sizeof(struct kvfile),
 .ft_read   = &vfsfile_dev_null_read,
 .ft_write  = &vfsfile_dev_null_write,
 .ft_pread  = &vfsfile_dev_null_pread,
 .ft_pwrite = &vfsfile_dev_null_pwrite,
 .ft_seek   = &vfsfile_dev_null_seek,
 .ft_trunc  = &vfsfile_dev_null_trunc,
 .ft_flush  = &vfsfile_dev_null_flush,
};




//////////////////////////////////////////////////////////////////////////
// === STD Files ===
static kerrno_t kvdev_fdopen(struct kfile *__restrict self, int fdno);
static kerrno_t dev_stdin_open(struct kfile *__restrict self, struct kdirent *__restrict __unused(dirent), struct kinode *__restrict __unused(inode), __openmode_t __unused(mode)) { return kvdev_fdopen(self,STDIN_FILENO); }
static kerrno_t dev_stdout_open(struct kfile *__restrict self, struct kdirent *__restrict __unused(dirent), struct kinode *__restrict __unused(inode), __openmode_t __unused(mode)) { return kvdev_fdopen(self,STDOUT_FILENO); }
static kerrno_t dev_stderr_open(struct kfile *__restrict self, struct kdirent *__restrict __unused(dirent), struct kinode *__restrict __unused(inode), __openmode_t __unused(mode)) { return kvdev_fdopen(self,STDERR_FILENO); }
struct kfiletype kvfiletype_dev_stdin  = { KVPROXYFILE_TYPE_INIT .ft_open = &dev_stdin_open };
struct kfiletype kvfiletype_dev_stdout = { KVPROXYFILE_TYPE_INIT .ft_open = &dev_stdout_open };
struct kfiletype kvfiletype_dev_stderr = { KVPROXYFILE_TYPE_INIT .ft_open = &dev_stderr_open };
static kerrno_t kvdev_fdopen(struct kfile *__restrict self, int fdno) {
 struct kfdentry entry;
 kerrno_t error;
 KTASK_CRIT_BEGIN
 error = kproc_getfd(kproc_self(),fdno,&entry);
 if __unlikely(KE_ISERR(error)) goto end;
 if __likely(entry.fd_type == KFDTYPE_FILE) {
  ((struct kvproxyfile *)self)->pf_used = entry.fd_file; // Inherit reference
 } else {
  error = KE_NOFILE;
  kfdentry_quit(&entry);
 }
end:
 KTASK_CRIT_END
 return error;
}


//////////////////////////////////////////////////////////////////////////
// TTY Terminal writer special file.
static kerrno_t
vfsfile_dev_tty_write(struct kfile *__restrict __unused(self),
                      void const *__restrict buf,
                      size_t bufsize, size_t *__restrict wsize) {
 tty_prints((char const *)buf,bufsize);
 *wsize = bufsize;
 return KE_OK;
}

struct kfiletype kvfiletype_dev_tty = {
 KVFILE_TYPE_INIT
 .ft_size  = sizeof(struct kvfile),
 .ft_write = &vfsfile_dev_tty_write,
};



//////////////////////////////////////////////////////////////////////////
// Declare all the inodes
struct kinode kvinode_dev_full    = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_full,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_kbevent = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_kbevent,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_kbkey   = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_kbkey,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_kbscan  = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_kbscan,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_kbtext  = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_kbtext,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_null    = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_null,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_stderr  = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_stderr,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_stdin   = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_stdin,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_stdout  = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_stdout,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_tty     = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_tty,(struct ksuperblock *)&kvfs_dev,S_IFCHR);
struct kinode kvinode_dev_zero    = KINODE_INIT(&kinode_generic_emptytype,&kvfiletype_dev_zero,(struct ksuperblock *)&kvfs_dev,S_IFCHR);


//////////////////////////////////////////////////////////////////////////
// The static portion of the '/dev' filesystem.
extern struct kvsdirsuperblock kvfs_dev;
static struct kvsdirent vfsent_dev[] = {
 KVDIRENT_INIT("full",   &kvinode_dev_full),
 KVDIRENT_INIT("kbevent",&kvinode_dev_kbevent),
 KVDIRENT_INIT("kbkey",  &kvinode_dev_kbkey),
 KVDIRENT_INIT("kbscan", &kvinode_dev_kbscan),
 KVDIRENT_INIT("kbtext", &kvinode_dev_kbtext),
 KVDIRENT_INIT("null",   &kvinode_dev_null),
 KVDIRENT_INIT("stderr", &kvinode_dev_stderr),
 KVDIRENT_INIT("stdin",  &kvinode_dev_stdin),
 KVDIRENT_INIT("stdout", &kvinode_dev_stdout),
 KVDIRENT_INIT("tty",    &kvinode_dev_tty),
 KVDIRENT_INIT("zero",   &kvinode_dev_zero),
 KVDIRENT_INIT_SENTINAL
};
struct kvsdirsuperblock kvfs_dev = KVSDIRSUPERBLOCK_INIT(vfsent_dev);


__DECL_END

#endif /* !__KOS_KERNEL_FS_VFS_DEV_C__ */
