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

/* Pseudo terminal device
 * Fun fact: TTY stands for 'Tele TYping machine' */

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



extern void kpty_init(struct kpty *__restrict self,
                      struct termios const *__restrict ios,
                      struct winsize const *__restrict size);
extern __crit void kpty_quit(struct kpty *__restrict self);
extern kerrno_t kpty_user_ioctl(struct kpty *__restrict self, kattr_t cmd, __user void *arg);

//////////////////////////////////////////////////////////////////////////
// Write data to the master (Usually keyboard input)
// @return: KE_OK:        Successfully written all given data.
// @return: KE_DESTROYED: The given PTY was destroyed.
// @return: KE_FAULT:     [kpty_user_mwrite*] A given pointer was faulty.
// @return: KE_TIMEDOUT:  A timeout previously set using alarm() has expired.
// @return: KE_INTR:      The calling thread was interrupted.
extern kerrno_t kpty_mwrite_unlocked(struct kpty *__restrict self, __kernel void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kpty_mwrite(struct kpty *__restrict self, __kernel void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kpty_user_mwrite_unlocked(struct kpty *__restrict self, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kpty_user_mwrite(struct kpty *__restrict self, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);

//////////////////////////////////////////////////////////////////////////
// Read data from perspective of the master (aka. output of slave)
// @return: KE_OK:        Successfully read all given data.
// @return: KE_DESTROYED: The given PTY was destroyed.
// @return: KE_FAULT:    [kpty_user_mread] A given pointer was faulty.
// @return: KE_TIMEDOUT:  A timeout previously set using alarm() has expired.
// @return: KE_INTR:      The calling thread was interrupted.
#define kpty_mread(self,buf,bufsize,rsize) \
 kiobuf_read(&(self)->ty_s2m,buf,bufsize,rsize,KIO_BLOCKFIRST)
#define kpty_user_mread(self,buf,bufsize,rsize) \
 kiobuf_user_read(&(self)->ty_s2m,buf,bufsize,rsize,KIO_BLOCKFIRST)

//////////////////////////////////////////////////////////////////////////
// Write data to the slave.
//  - Usually text that meant as output to the terminal.
//  - Usually allowed to contain control characters.
// @return: KE_OK:        Successfully wrote the given data.
// @return: KE_DESTROYED: The given PTY was destroyed.
// @return: KE_TIMEDOUT:  A timeout previously set using alarm() has expired.
// @return: KE_INTR:      The calling task was interrupted.
// @return: KE_FAULT:    [kpty_user_swrite] A given pointer was faulty.
extern kerrno_t kpty_swrite(struct kpty *__restrict self, __kernel void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kpty_user_swrite(struct kpty *__restrict self, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);

//////////////////////////////////////////////////////////////////////////
// Read data from perspective of the slave.
//  - Usually processed & filtered keyboard input.
// @return: KE_OK:        Successfully wrote the given data.
// @return: KE_DESTROYED: The given PTY was destroyed.
// @return: KE_TIMEDOUT:  A timeout previously set using alarm() has expired.
// @return: KE_INTR:      The calling task was interrupted.
// @return: KE_FAULT:    [kpty_user_sread] A given pointer was faulty.
// @return: KS_EMPTY:     The associated I/O buffer was interrupted using
//                        'kiobuf_interrupt' ('*rsize' is set to ZERO(0)).
//                        This usually happens if the PTY was disconnected,
//                        such as a remote connection being reset, causing
//                        a blocking read operation in the slave process to
//                        be interrupted.
extern kerrno_t kpty_user_sread(struct kpty *__restrict self, __user void *buf,
                                __size_t bufsize, __kernel __size_t *__restrict rsize);
#ifdef __INTELLISENSE__
extern kerrno_t kpty_sread(struct kpty *__restrict self, __kernel void *buf,
                           __size_t bufsize, __kernel __size_t *__restrict rsize);
#else
#define kpty_sread(self,buf,bufsize,rsize) KTASK_KEPD(kpty_user_sread(self,buf,bufsize,rsize))
#endif


struct kfspty {
 // A PTY wrapped within an INode
 struct kinode fp_node; /*< Underlying INode. */
 struct kpty   fp_pty;  /*< Associated PTY. */
};

//////////////////////////////////////////////////////////////////////////
// Get the attributes of of a given PTY.
// NOTE: For convenience, PTY devices implement the 'KATTR_FS_SIZE'
//       interface, allowing you to query a hint for a required
//       buffer size when reading large portions of data.
// @return: KE_OK:     Successfully read all given attributes.
// @return: KE_FAULT: [kfspty_user_*] A given pointer was faulty.
// @return: * :        Some other attribute-secific error occurred
//                    (s.a. kinode_user_generic_(g|s)etattr).
extern __wunused __nonnull((1)) kerrno_t
kfspty_user_mgetattr(struct kfspty const *__restrict self,
                     __size_t ac, __user union kinodeattr av[]);
extern __wunused __nonnull((1)) kerrno_t
kfspty_user_sgetattr(struct kfspty const *__restrict self,
                     __size_t ac, __user union kinodeattr av[]);


//////////////////////////////////////////////////////////////////////////
// Create a new file-system node meant for a PTY (Pseudo terminal)
// @return: * :   A reference to a new Inode containing a PTY.
// @return: NULL: Not enough available memory.
extern __crit __ref struct kfspty *
kfspty_new(struct termios const *__restrict ios,
           struct winsize const *__restrict size);

struct kdirent;
//////////////////////////////////////////////////////////////////////////
// Insert a given Filesystem PTY into the actual file-system.
// @return: KE_OK:       The PTY was successfully inserted.
// @return: KE_OVERFLOW: Too many PTYs were already registered.
// @return: KE_NOMEM:    Not enough available memory.
// @return: KE_EXISTS:   The given PTY was already registered.
// @return: KE_FAULT:    'master_name_buf' is neither NULL, nor a valid pointer.
extern __crit __nonnull((1,2,3)) kerrno_t
kfspty_insnod(struct kfspty *__restrict self,
              __ref struct kdirent **__restrict master_ent,
              __ref struct kdirent **__restrict slave_ent,
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
extern __crit __wunused __malloccall __nonnull((1)) __ref struct kptyfile *
kptyfile_slave_new(struct kfspty *__restrict pty, struct kdirent *dent);
extern __crit __wunused __malloccall __nonnull((1)) __ref struct kptyfile *
kptyfile_master_new(struct kfspty *__restrict pty, struct kdirent *dent);





__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_PTY_H__ */
