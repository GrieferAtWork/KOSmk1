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
#ifndef __DIRENT_H__
#ifndef _DIRENT_H
#define __DIRENT_H__ 1
#define _DIRENT_H 1

#include <kos/compiler.h>
#include <kos/types.h>

#ifndef __ASSEMBLY__
__DECL_BEGIN

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

//////////////////////////////////////////////////////////////////////////
// Enable support for directory entires with names longer than NAME_MAX
// NOTE: This only affects 'readdir'. - 'readdir_r' is unaffected and
//       will still be bound to a maximum name length of 'NAME_MAX'
// WARNING: If your code depends on filenames read from directories,
//          as found within the 'd_name' field not being longer than
//          'NAME_MAX', either re-compile libc with this option disabled,
//          or simply put the following code after #including <dirent.h>:
//       >> #if __DIRENT_HAVE_LONGENT
//       >> #define readdir   _readdir_short
//       >> #endif
#ifndef __DIRENT_HAVE_LONGENT
#define __DIRENT_HAVE_LONGENT 1
#endif

struct dirent {
    __ino_t       d_ino;              /*< file serial number. */
    unsigned char d_type;             /*< One of the 'DT_*' constants below. */
    __size_t      d_namlen;           /*< Length of the filename below. */
    char          d_name[NAME_MAX+1]; /*< name of entry. */
};

struct __dirstream {
    int            __d_fd;      /*< Directory file descriptor. */
    struct dirent  __d_ent;     /*< Inline-allocated directory entry returned from readdir by default. */
#if __DIRENT_HAVE_LONGENT
    // NOTE: The long dirent is used if the filename of any directory entry exceeds 'NAME_MAX'
    struct dirent *__d_longent; /*< [1..1] Used dirent (if != &__d_ent, must be freed using 'free()') */
    __size_t       __d_longsz;  /*< Size of the long dirent. */
#endif /* __DIRENT_HAVE_LONGENT */
};

#undef  _DIRENT_HAVE_D_RECLEN
#undef  _DIRENT_HAVE_D_OFF
#define _DIRENT_HAVE_D_NAMLEN
#define _DIRENT_HAVE_D_TYPE
#define _D_EXACT_NAMLEN(d) ((d)->d_namlen)
#define _D_ALLOC_NAMLEN(d) (_D_EXACT_NAMLEN(d)+1)

#define DT_UNKNOWN 0
#define DT_FIFO    1
#define DT_CHR     2
#define DT_DIR     4
#define DT_BLK     6
#define DT_REG     8
#define DT_LNK     10
#define DT_SOCK    12
#define DT_WHT     14

/* Convert between stat structure types and directory types.  */
#define IFTODT(mode)    (((mode) & 0170000) >> 12)
#define DTTOIF(dirtype) ((dirtype) << 12)

typedef struct __dirstream DIR;

#ifndef __CONFIG_MIN_LIBC__
#undef readdir
extern                        __nonnull((1))     int closedir __P((DIR *__restrict __dirp));
extern __wunused __malloccall __nonnull((1))     DIR *opendir __P((char const *__restrict __dirname));
extern __wunused __malloccall                    DIR *fdopendir __P((int __fd));
extern __wunused              __nonnull((1))     struct dirent *readdir __P((DIR *__restrict __dirp));
extern __wunused              __nonnull((1,2,3)) int readdir_r __P((DIR *__restrict __dirp, struct dirent *__restrict __entry, struct dirent **__restrict __result));
extern                        __nonnull((1))     void rewinddir __P((DIR *__restrict __dirp));
extern                        __nonnull((1))     void seekdir __P((DIR *__restrict __dirp, long __loc));
extern __wunused              __nonnull((1))     long telldir __P((DIR *__restrict __dirp));
extern __wunused              __nonnull((1))     int dirfd __P((DIR *__restrict __dirp));
#if __DIRENT_HAVE_LONGENT
// Legacy-style directory reading (truncate filenames longer than NAME_MAX)
extern __wunused              __nonnull((1))     struct dirent *_readdir_short(DIR *__restrict __dirp);
#endif /* __DIRENT_HAVE_LONGENT */
#endif /* !__CONFIG_MIN_LIBC__ */

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_DIRENT_H */
#endif /* !__DIRENT_H__ */
