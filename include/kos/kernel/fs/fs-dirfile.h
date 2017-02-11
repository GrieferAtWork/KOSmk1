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
#ifndef __KOS_KERNEL_FS_FS_DIRFILE_H__
#define __KOS_KERNEL_FS_FS_DIRFILE_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/types.h>
#include <kos/kernel/mutex.h>

__DECL_BEGIN

struct kdirfileentry {
 __ref struct kinode  *dfe_inode;  /*< [1..1] Associated node. */
 struct kdirentname    dfe_name;   /*< Associated name (NOTE: When 'dfe_dirent' is NULL, this field is owned). */
 __ref struct kdirent *dfe_dirent; /*< [0..1] Associated dirent (or NULL if now known). */
};
#define kdirfileentry_quit(self) \
 __xblock({ struct kdirfileentry *const __dfeself = (self);\
            kinode_decref(__dfeself->dfe_inode); \
            if (__dfeself->dfe_dirent) {\
             kdirent_decref(__dfeself->dfe_dirent); \
            } else {\
             kdirentname_quit(&__dfeself->dfe_name);\
            }\
            (void)0;\
 })


#define KDIRFILE_ENTV_EMPTY  \
 ((struct kdirfileentry *)(uintptr_t)-1) /*< Value after enumeration of empty directory. */
struct kdirfile {
 struct kfile          df_file;   /*< Underlying file. */
 __ref struct kdirent *df_dirent; /*< [1..1][const] Directory entry associated with this file. */
 __ref struct kinode  *df_inode;  /*< [1..1][const] INode associated with this file. */
 struct kmutex         df_lock;   /*< Lock. */
 struct kdirfileentry *df_curr;   /*< [lock(df_lock)] Current dirent vector position. */
 struct kdirfileentry *df_entv;   /*< [0..df_endv][owned] Vector of loaded directory entries. */
 struct kdirfileentry *df_endv;   /*< [lock(df_lock)] End of the dirent vector. */
};
#define KDIRFILE_INIT(dirent,inode)\
 {KFILE_INIT(&kdirfile_type),dirent,inode,KMUTEX_INIT,NULL,NULL,NULL}

//////////////////////////////////////////////////////////////////////////
// Allocate a new root directory file
extern __ref struct kdirfile *kdirfile_newroot(void);

// The file type for pretty much all directories.
extern struct kfiletype kdirfile_type;

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_FS_DIRFILE_H__ */
