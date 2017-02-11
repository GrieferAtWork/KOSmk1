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
#ifndef __KOS_ATTR_H__
#define __KOS_ATTR_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/timespec.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
typedef __u16        kattrtoken_t; /* 9 bit */
typedef __u8         kattrgroup_t; /* 7 bit */
typedef __u8         kattrid_t;    /* 8 bit */
typedef __u8         kattrver_t;   /* 8 bit */
typedef unsigned int kattr_t;
#endif

#ifndef __ASSEMBLY__
#define KATTR_ENCODEVER(T)                  (sizeof(T)%0x7f)
#define KATTR_ENCODEVER_VAR(Telem)         ((sizeof(Telem)%0x7f)|0x80)
#define KATTR_ENCODEVER_BVAR(Tbase,Telem) (((sizeof(Tbase)+sizeof(Telem))%0x7f)|0x80)

#define KATTR(token,group,id,T) \
 ((((token)&0x1ff) << 23) |\
  (((group)& 0x7f) << 16) |\
     (((id)& 0xff) << 8) |\
   KATTR_ENCODEVER(T))
#define KATTR_VAR(token,group,id,Telem) \
 ((((token)&0x1ff) << 23) |\
  (((group)& 0x7f) << 16) |\
     (((id)& 0xff) << 8) |\
   KATTR_ENCODEVER_VAR(Telem))
#define KATTR_BVAR(token,group,id,Tbase,Telem) \
 ((((token)&0x1ff) << 23) |\
  (((group)& 0x7f) << 16) |\
     (((id)& 0xff) << 8) |\
  KATTR_ENCODEVER_BVAR(Tbase,Telem))

#define KATTR_GETTOKEN(attr) ((kattrtoken_t)(((attr)&0xff800000) >> 23))
#define KATTR_GETGROUP(attr) ((kattrgroup_t)(((attr)&0x007f0000) >> 16))
#define KATTR_GETID(attr)    ((kattrid_t)(((attr)&0x0000ff00) >> 8))
#define KATTR_GETVER(attr)   ((kattrver_t)((attr)&0x000000ff))
#define KATTR_ISVAR(attr)    (((attr)&0x00000080)!=0)
#define KATTR_ISBVAR(attr)   (((attr)&0x000000C0)!=0)

#define KATTR_TOKEN(id)  id

// Attribute tokens
#define KATTR_TOKEN_NONE KATTR_TOKEN(0)
#define KATTR_TOKEN_DEV  KATTR_TOKEN(1)
#define KATTR_TOKEN_FS   KATTR_TOKEN(2)
#define KATTR_TOKEN_MP   KATTR_TOKEN(3)

#define KATTR_GROUP_GENERIC_PROP 'P'
// Retrieve a generic, human-readable name for the associated object.
// NOTE: This is the attribute used to specify task names.
// NOTE: For many objects, this attribute retrives data also accessible
//       through other attributes (e.g.: 'KATTR_FS_FILENAME')
// NOTE: To set the name of task, this attribute is used as well.
#define KATTR_GENERIC_NAME  KATTR_VAR(KATTR_TOKEN_NONE,KATTR_GROUP_GENERIC_PROP,0,char) /*< "Peter". */

#define KATTR_GROUP_DEV_GENERIC  'G'
#define KATTR_GROUP_DEV_STORAGE  'S'

//////////////////////////////////////////////////////////////////////////
// [RO] Retrieve a human-readable device-exclusive/type name.
//      @param: buf:     [get] The zero-terminated device name.
//      @param: bufsize: [get] The amount of available bytes in 'buf' to write to.
//      @param: reqsize: [get] The size of the name in bytes (including the \0 character)
#define KATTR_DEV_NAME    KATTR_VAR(KATTR_TOKEN_DEV,KATTR_GROUP_DEV_GENERIC,0,char)
#define KATTR_DEV_TYPE    KATTR_VAR(KATTR_TOKEN_DEV,KATTR_GROUP_DEV_GENERIC,1,char)

#define KATTR_GROUP_FS_SPECS    'S' /*< Filesystem Attribute group for filesystem specifications. */
#define KATTR_FS_BLOCKSIZE  KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_SPECS,0,__size_t)
#define KATTR_FS_BLOCKCNT   KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_SPECS,1,__size_t)

#define KATTR_GROUP_FS_FILE     'F' /*< File attribute group. */
#define KATTR_FS_FILENAME   KATTR_VAR(KATTR_TOKEN_FS,KATTR_GROUP_FS_FILE,0,char) /*< "foo.txt". */
#define KATTR_FS_PATHNAME   KATTR_VAR(KATTR_TOKEN_FS,KATTR_GROUP_FS_FILE,1,char) /*< "/tmp/foo.txt". */

#define KATTR_GROUP_FS_METADATA 'M' /*< Filesystem Attribute group for file metadata. */
#define KATTR_FS_ATTRS  KATTR_VAR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,0,union kinodeattr) /*< Get/Set multiple attributes at once. */
#define KATTR_FS_ATIME  KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,1,struct timespec)
#define KATTR_FS_CTIME  KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,2,struct timespec)
#define KATTR_FS_MTIME  KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,3,struct timespec)
#define KATTR_FS_PERM   KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,4,__mode_t) /*< Only Permissions. */
#define KATTR_FS_OWNER  KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,5,__uid_t)
#define KATTR_FS_GROUP  KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,6,__gid_t)
#define KATTR_FS_SIZE   KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,7,__pos_t) // READ-ONLY
#define KATTR_FS_INO    KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,8,__ino_t) // READ-ONLY
#define KATTR_FS_NLINK  KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,9,__nlink_t) // READ-ONLY (When not supported, you should return '1' in 'n_lnk')
#define KATTR_FS_KIND   KATTR(KATTR_TOKEN_FS,KATTR_GROUP_FS_METADATA,10,__mode_t) /*< Only kind. */


#define KINODEATTR_HEAD kattr_t a_id; /*< Attribute ID. */
struct kinodeattr_common { KINODEATTR_HEAD };
struct kinodeattr_time { KINODEATTR_HEAD struct timespec tm_time; };
struct kinodeattr_perm { KINODEATTR_HEAD __mode_t p_perm; };
struct kinodeattr_owner { KINODEATTR_HEAD __uid_t o_owner; };
struct kinodeattr_group { KINODEATTR_HEAD __gid_t g_group; };
struct kinodeattr_size { KINODEATTR_HEAD __pos_t sz_size; };
struct kinodeattr_ino { KINODEATTR_HEAD __ino_t i_ino; };
struct kinodeattr_nlink { KINODEATTR_HEAD __nlink_t n_lnk; };
struct kinodeattr_kind { KINODEATTR_HEAD __mode_t k_kind; };
#undef KINODEATTR_HEAD
union kinodeattr {
 struct kinodeattr_common ia_common;
 struct kinodeattr_time   ia_time;
 struct kinodeattr_perm   ia_perm;
 struct kinodeattr_owner  ia_owner;
 struct kinodeattr_group  ia_group;
 struct kinodeattr_size   ia_size;
 struct kinodeattr_ino    ia_ino;
 struct kinodeattr_nlink  ia_nlink;
 struct kinodeattr_kind   ia_kind;
 __u8 __padding[16]; /*< Pad to 16 bytes. */
};

#if 0
#define KATTR_GROUP_MP_TASK 'T' /*< Multi-threading task-specific attributes. */
struct kmpstack {
 __pagealigned __user void *ms_base;  /*< Base of the stack. */
 __size_t                   ms_size;  /*< Size of the stack. */
#define KMPSTACK_FLAG_NONE 0x00000000
#define KMPSTACK_FLAG_FREE 0x00000002 /*< The stack will be munmap-ed upon task termination. */
 __u32                      ms_flags; /*< Stack flags. */
};
#define KATTR_MP_STACK  KATTR(KATTR_TOKEN_MP,KATTR_GROUP_MP_TASK,0,struct kmpstack) /*< Get/Set the user-stack of a thread (e.g.: On i386, this is its %esp) */
#endif

#endif /* __ASSEMBLY__ */

__DECL_END

#endif /* !__KOS_ATTR_H__ */
