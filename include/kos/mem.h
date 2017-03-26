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
#ifndef __KOS_MEM_H__
#define __KOS_MEM_H__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/types.h>

#define KMEM_MAP_FAIL   ((void *)(__uintptr_t)-1)
#ifndef __NO_PROTOTYPES
#ifndef __KERNEL__
#include <kos/syscall.h>
#include <kos/errno.h>


__DECL_BEGIN

#ifdef __WANT_KERNEL_MEM_VALIDATE
//////////////////////////////////////////////////////////////////////////
// Validates a given memory range to describe valid user memory
// @return: KE_OK:        Every thing's fine!
// @return: KS_EMPTY:     The NULL pointer is part of the given range.
// @return: KE_FAULT:     Apart of the given range isn't mapped.
// @return: KE_INVAL:     The given memory describes a portion of memory that is not allocated.
// @return: KE_RANGE:     The given memory doesn't map to any valid RAM/Device memory.
// @return: KE_DESTROYED: The calling process is currently being destroyed (can't access the page directory).
__local _syscall2(kerrno_t,kmem_validate,void const *__restrict,addr,__size_t,bytes);
#endif

//////////////////////////////////////////////////////////////////////////
// Request the kernel kernel to map at least 'length' bytes of physical
// memory to the calling process, starting at 'physptr'.
// Depending on the range of memory requested, this function may
// fail with various kinds of errors.
// It should be obvious that you can't just map any memory, with this
// function being meant to map device memory, such as the x86-VGA terminal.
// - You can never map memory if you're not allowed to.
// TODO: @return
__local _syscall5(kerrno_t,kmem_mapdev,void **,hint_and_result,
                  __size_t,length,int,prot,int,flags,void *,physptr);

//////////////////////////////////////////////////////////////////////////
// TODO: DOC
// @return: KE_ISERR(*) : '*__hint_and_result' is set to KMEM_MAP_FAIL.
__local kerrno_t kmem_map
__D6(void **,__hint_and_result,__size_t,__length,
     int,__prot,int,__flags,int,__fd,__u64,__off) {
 kerrno_t __error;
 if (__fd == -1) {
  __asm_volatile__(__ASMSYSCALL_DOINT
                   : "=a" (__error)
                   , "+c" (*__hint_and_result)
                   : "a" (SYS_kmem_map)
                   , "d" (__length)
                   , "b" ((__u32)__prot << 16 |(__u32)__flags)
                   , "S" (0)
                   : "memory");
 } else {
  struct { int __fd; __u64 __off; } __finfo = {__fd,__off};
  __asm_volatile__(__ASMSYSCALL_DOINT
                   : "=a" (__error)
                   , "+c" (*__hint_and_result)
                   : "a" (SYS_kmem_map)
                   , "d" (__length)
                   , "b" ((__u32)__prot << 16 |(__u32)__flags)
                   , "S" (&__finfo)
                   : "memory");
 }
 return __error;
}

__local _syscall2(kerrno_t,kmem_unmap,void *,addr,__size_t,length);

__DECL_END
#endif /* !__KERNEL__ */
#endif /* !__NO_PROTOTYPES */


#endif /* !__KOS_MEM_H__ */
