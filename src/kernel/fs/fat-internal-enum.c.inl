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
#ifndef __KOS_KERNEL_FS_FAT_INTERNAL_ENUM_C_INL__
#define __KOS_KERNEL_FS_FAT_INTERNAL_ENUM_C_INL__ 1

#include <alloca.h>
#include <ctype.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/kernel/types.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>

__DECL_BEGIN

struct check_short_data {
 char const *short_name;
 char const *__restrict long_name;
 size_t long_name_size;
};

static kerrno_t
check_short_callback(struct kfatfs *__restrict fs, struct kfatfilepos const *__restrict fpos,
                     struct kfatfileheader const *__restrict file,
                     char const *filename, size_t filename_size,
                     struct check_short_data *data) {
 if (!memcmp(data->short_name,file->f_nameext,KFATFILE_NAMEMAX+KFATFILE_EXTMAX))
  return KE_EXISTS; /* Short name already exists. */
 if (filename_size == data->long_name_size &&
     !memcmp(data->long_name,filename,filename_size)
     ) return KE_NOENT; /* Long name already exists. */
 return KE_OK;
}


kerrno_t
kfatfs_checkshort(struct kfatfs *__restrict self, kfatcls_t dir, int dir_is_sector,
                  char const *__restrict long_name, size_t long_name_size,
                  char const name[KFATFILE_NAMEMAX+KFATFILE_EXTMAX]) {
 struct check_short_data data;
 data.long_name = long_name;
 data.long_name_size = long_name_size;
 data.short_name = (char const *)name;
 return  dir_is_sector
  ? kfatfs_enumdirsec(self,dir,self->f_rootmax,(pkfatfs_enumdir_callback)&check_short_callback,&data)
  : kfatfs_enumdir   (self,dir,(pkfatfs_enumdir_callback)&check_short_callback,&data);
}

__DECL_END

#ifndef __INTELLISENSE__
#define DIRISSEC
#include "fat-internal-enum-impl.c.inl"
#include "fat-internal-enum-impl.c.inl"
#endif

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_ENUM_C_INL__ */
