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
#ifndef __KOS_KERNEL_FS_VFS_DEV_H__
#define __KOS_KERNEL_FS_VFS_DEV_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/vfs.h>

__DECL_BEGIN

/* "/dev": The /dev filesystem superblock. */
extern struct kvsdirsuperblock kvfs_dev;

/* "/dev/tty": TTY terminal. */
extern struct kfiletype kvfiletype_dev_tty;
extern struct kinode    kvinode_dev_tty;

/* "/dev/kb*": Keyboard files. */
extern struct kfiletype kvfiletype_dev_kbevent; /*< FULL: Keyboard event (struct kbevent). */
extern struct kfiletype kvfiletype_dev_kbkey;   /*< Translated: 2-byte kbkey_t. */
extern struct kfiletype kvfiletype_dev_kbscan;  /*< RAW:  Hardware scancodes (kbscan_t). */
extern struct kfiletype kvfiletype_dev_kbtext;  /*< Translated: 1-byte UTF-8 char. */
extern struct kinode    kvinode_dev_kbevent;
extern struct kinode    kvinode_dev_kbkey;
extern struct kinode    kvinode_dev_kbscan;
extern struct kinode    kvinode_dev_kbtext;

/* "/dev/null", ...: Helper files. */
extern struct kfiletype kvfiletype_dev_full;
extern struct kfiletype kvfiletype_dev_zero;
extern struct kfiletype kvfiletype_dev_null;
extern struct kinode    kvinode_dev_full;
extern struct kinode    kvinode_dev_zero;
extern struct kinode    kvinode_dev_null;

/* "/dev/std*": STD-files. */
extern struct kfiletype kvfiletype_dev_stdin;
extern struct kfiletype kvfiletype_dev_stdout;
extern struct kfiletype kvfiletype_dev_stderr;
extern struct kinode    kvinode_dev_stdin;
extern struct kinode    kvinode_dev_stdout;
extern struct kinode    kvinode_dev_stderr;

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_VFS_DEV_H__ */
