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
#ifndef __KOS_KERNEL_PTY_H__
#define __KOS_KERNEL_PTY_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/iobuf.h>
#include <kos/kernel/object.h>
#include <kos/kernel/rawbuf.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/file.h>
#include <sys/termios.h>

__DECL_BEGIN

//
// Pseudo terminal device
// Fun fact: TTY stands for 'Tele TYping machine'
//

#define KOBJECT_MAGIC_PTY 0x971 /*< PTY. */
#define kassert_kpty(self) kassert_object(self,KOBJECT_MAGIC_PTY)

struct kproc;

#define PTY_IO_BUFFER_SIZE   4096
#define PTY_LINE_BUFFER_SIZE 4096

typedef __u32 kptynum_t;

struct kpty {
 KOBJECT_HEAD
 kptynum_t           ty_num;   /*< PTY Number. (== (kptynum_t)-1 when not set) */
 struct kiobuf       ty_s2m;   /*< Slave --> Master comunication buffer. (out) */
 struct kiobuf       ty_m2s;   /*< Master --> Slave comunication buffer. (in) */
 struct krwlock      ty_lock;  /*< R/W lock for members below. */
 struct krawbuf      ty_canon; /*< [lock(ty_lock)] Canon (aka. Line) buffer. */
 struct winsize      ty_size;  /*< [lock(ty_lock)] Terminal window size. */
 struct termios      ty_ios;   /*< [lock(ty_lock)] termios data. */
 __ref struct kproc *ty_cproc; /*< [lock(ty_lock)][0..1] Controlling process. */
 __ref struct kproc *ty_fproc; /*< [lock(ty_lock)][0..1] Foreground process. */
};



extern void kpty_init(struct kpty *self,
                      struct termios const *ios,
                      struct winsize const *size);
extern __crit void kpty_quit(struct kpty *self);
extern kerrno_t kpty_ioctl(struct kpty *self, kattr_t cmd, __user void *arg);

// Write data to the master (Usually keyboard input)
extern kerrno_t kpty_mwrite_unlocked(struct kpty *__restrict self, void const *buf, __size_t bufsize, __size_t *__restrict wsize);
extern kerrno_t kpty_mwrite(struct kpty *__restrict self, void const *buf, __size_t bufsize, __size_t *__restrict wsize);

// Read data from perspective of the master (aka. output of slave)
#define kpty_mread(self,buf,bufsize,rsize) \
 kiobuf_read(&(self)->ty_s2m,buf,bufsize,rsize,KIO_BLOCKFIRST)

// Write data to the slave (Usually text that meant as output to the terminal; usually allowed to contain control characters)
extern kerrno_t kpty_swrite(struct kpty *__restrict self, void const *buf, __size_t bufsize, __size_t *__restrict wsize);

// Read data from perspective of the slave (usually processed & filtered keyboard input)
extern kerrno_t kpty_sread(struct kpty *__restrict self, void *buf, __size_t bufsize, __size_t *__restrict rsize);


struct kfspty {
 // A PTY wrapped within an INode
 struct kinode fp_node; /*< Underlying INode. */
 struct kpty   fp_pty;  /*< Associated PTY. */
};

extern kerrno_t kfspty_mgetattr(struct kfspty const *self, __size_t ac, union kinodeattr *av);
extern kerrno_t kfspty_sgetattr(struct kfspty const *self, __size_t ac, union kinodeattr *av);


//////////////////////////////////////////////////////////////////////////
// Create a new file-system node meant for a PTY (Pseudo terminal)
// @return: * :   A reference to a new Inode containing a PTY.
// @return: NULL: Not enough available memory.
extern __crit __ref struct kfspty *
kfspty_new(struct termios const *ios,
           struct winsize const *size);

struct kdirent;
//////////////////////////////////////////////////////////////////////////
// Insert a given Filesystem PTY into the actual file-system.
// @return: KE_OK:       The PTY was successfully inserted.
// @return: KE_OVERFLOW: Too many PTYs were already registered.
// @return: KE_NOMEM:    Not enough available memory.
// @return: KE_EXISTS:   The given PTY was already registered.
// @return: KE_FAULT:    'master_name_buf' is neither NULL, nor a valid pointer.
extern __crit kerrno_t
kfspty_insnod(struct kfspty *self,
              __ref struct kdirent **master_ent,
              __ref struct kdirent **slave_ent,
              __user char *master_name_buf);

extern struct kinodetype kfspty_type;


struct kptyfile {
 struct kfile          pf_file; /*< Underlying file object. */
 __ref struct kfspty  *pf_pty;  /*< [1..1][const] Filesystem PTY node. */
 __ref struct kdirent *pf_dent; /*< [0..1][const] Filesystem PTY directory entry. */
};
extern struct kfiletype kptyfile_slave_type;
extern struct kfiletype kptyfile_master_type;

//////////////////////////////////////////////////////////////////////////
// Create a new master/slave file for a given Filesystem PTY node.
// @return: * :   A reference to a new file.
// @return: NULL: Not enough available memory.
extern __nonnull((1)) __ref struct kptyfile *
kptyfile_slave_new(struct kfspty *__restrict pty,
                   struct kdirent *dent);
extern __nonnull((1)) __ref struct kptyfile *
kptyfile_master_new(struct kfspty *__restrict pty,
                    struct kdirent *dent);





__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_PTY_H__ */
