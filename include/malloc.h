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
#ifndef __MALLOC_H__
#ifndef _MALLOC_H
#define __MALLOC_H__ 1
#define _MALLOC_H 1

#include <__null.h>
#include <kos/config.h>
#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/types.h>
#ifdef __LIBC_HAVE_DEBUG_MALLOC
#include <kos/kernel/debug.h>
#endif

#ifdef __COMPILER_HAVE_PRAGMA_PUSH_MACRO
#ifdef calloc
#define __malloc_h_calloc_defined
#pragma push_macro("calloc")
#endif
#ifdef free
#define __malloc_h_free_defined
#pragma push_macro("free")
#endif
#ifdef malloc
#define __malloc_h_malloc_defined
#pragma push_macro("malloc")
#endif
#ifdef malloc_usable_size
#define __malloc_h_malloc_usable_size_defined
#pragma push_macro("malloc_usable_size")
#endif
#ifdef mallopt
#define __malloc_h_mallopt_defined
#pragma push_macro("mallopt")
#endif
#ifdef realloc
#define __malloc_h_realloc_defined
#pragma push_macro("realloc")
#endif
#ifdef cfree
#define __malloc_h_cfree_defined
#pragma push_macro("cfree")
#endif
#ifdef memalign
#define __malloc_h_memalign_defined
#pragma push_macro("memalign")
#endif
#ifdef aligned_alloc
#define __malloc_h_aligned_alloc_defined
#pragma push_macro("aligned_alloc")
#endif
#ifdef posix_memalign
#define __malloc_h_posix_memalign_defined
#pragma push_macro("posix_memalign")
#endif
#ifdef pvalloc
#define __malloc_h_pvalloc_defined
#pragma push_macro("pvalloc")
#endif
#ifdef valloc
#define __malloc_h_palloc_defined
#pragma push_macro("valloc")
#endif
#endif

/* Kill user-defined malloc macros. */
#undef calloc
#undef free
#undef malloc
#undef malloc_usable_size
#undef mallopt
#undef realloc
#undef cfree
#undef memalign
#undef aligned_alloc
#undef pvalloc
#undef valloc

__DECL_BEGIN

#ifdef PAGESIZE
#   define __ATTRIBUTE_ALIGNED_PAGE __attribute_aligned_c(PAGESIZE)
#elif defined(PAGE_SIZE)
#   define __ATTRIBUTE_ALIGNED_PAGE __attribute_aligned_c(PAGE_SIZE)
#elif defined(__i386__) && __SIZEOF_POINTER__ >= 4
#   define __ATTRIBUTE_ALIGNED_PAGE __attribute_aligned_c(4096)
#else
#   define __ATTRIBUTE_ALIGNED_PAGE /* nothing */
#endif
#if __SIZEOF_POINTER__ == 8
#   define __ATTRIBUTE_ALIGNED_DEFAULT __attribute_aligned_c(16)
#elif __SIZEOF_POINTER__ == 4
#   define __ATTRIBUTE_ALIGNED_DEFAULT __attribute_aligned_c(8)
#elif __SIZEOF_POINTER__ == 2
#   define __ATTRIBUTE_ALIGNED_DEFAULT __attribute_aligned_c(4)
#else
#   define __ATTRIBUTE_ALIGNED_DEFAULT /* nothing */
#endif

#ifndef __MALLOC_C__
#if defined(__CONFIG_MIN_LIBC__) && !defined(__INTELLISENSE__)
#   define cfree              free
#elif !defined(__STDC_PURE__)
extern __crit void cfree __P((void *__restrict __mallptr)) __asmname("free");
#endif

// C11/C++11 aligned_alloc (it's literally memalign...)
// NOTE: The standard says this should fail if 'BYTES%ALIGNMENT != 0'
//      (aka. 'BYTES' is a multiple of 'ALIGNMENT').
//       Now if that isn't a bunch of bull, I don't know what is.
//       So the default libc implementation of KOS doesn't
//       support that *quote* 'feature'...
#ifdef __INTELLISENSE__
extern __crit __sizemalloccall((2)) __attribute_aligned_a(1)
void *aligned_alloc(__size_t __alignment, __size_t __bytes);
#elif !defined(__CONFIG_MIN_LIBC__)
extern __crit __sizemalloccall((2)) __attribute_aligned_a(1)
void *aligned_alloc __P((__size_t __alignment, __size_t __bytes)) __asmname("memalign");
#ifdef __LIBC_HAVE_DEBUG_MALLOC
#   define aligned_alloc  memalign
#endif
#else
#   define aligned_alloc  memalign
#endif
#endif /* !__MALLOC_C__ */


//////////////////////////////////////////////////////////////////////////
// Malloc implementation
// Implementation notes:
//  - libk use dlmalloc build on top of <kos/kernel/pageframe.h>
//  - libc use dlmalloc build on top of <sys/mman.h>
//  - malloc(0) does NOT return NULL, but some small, non-empty block of memory.
//  - realloc(p,0) does NOT act as free, but active like free+malloc.
//  - free() Never modifies the currently set value of 'errno'.
extern __crit __wunused __sizemalloccall((1)) __ATTRIBUTE_ALIGNED_DEFAULT void *malloc __P((__size_t __bytes));
extern __crit __wunused __sizemalloccall((1,2)) __ATTRIBUTE_ALIGNED_DEFAULT void *calloc __P((__size_t __count, __size_t __bytes));
extern __crit __wunused __sizemalloccall((2)) __attribute_aligned_a(1) void *memalign __P((__size_t __alignment, __size_t __bytes));
extern __crit __wunused __sizemalloccall((2)) __nonnull((1)) __ATTRIBUTE_ALIGNED_DEFAULT void *realloc __P((void *__restrict __mallptr, __size_t __bytes));
extern __crit __nonnull((1)) void free __P((void *__restrict __freeptr));
extern __crit __nonnull((1)) int posix_memalign __P((void **__restrict __memptr, __size_t __alignment, __size_t __bytes));
extern __crit __wunused __sizemalloccall((1)) __ATTRIBUTE_ALIGNED_PAGE void *pvalloc __P((__size_t __bytes));
extern __crit __wunused __sizemalloccall((1)) __ATTRIBUTE_ALIGNED_PAGE void *valloc __P((__size_t __bytes));

//////////////////////////////////////////////////////////////////////////
// Object-malloc:
// >> T *_omalloc(typename T);
// >> T *_ocalloc(typename T);
// These are merely a hand full of helper functions wrapping around
// malloc/calloc, designed to ease allocation of c-style structures:
// >> struct point {
// >>   int x;
// >>   int y;
// >> };
// >> #define point_new()        omalloc(struct point)
// >> #define point_delete(self) free(self)
#define _omalloc(T)    ((T *)malloc(sizeof(T)))
#define _ocalloc(T)    ((T *)calloc(1,sizeof(T)))

#ifndef __STDC_PURE__
#define omalloc   _omalloc
#define ocalloc   _ocalloc
#endif

//////////////////////////////////////////////////////////////////////////
// Configure a malloc parameter from dlmalloc
extern int mallopt __P((int __parameter_number,
                        int __parameter_value));
#define M_TRIM_THRESHOLD   (-1)
#define M_GRANULARITY      (-2)
#define M_MMAP_THRESHOLD   (-3)

//////////////////////////////////////////////////////////////////////////
// Release free memory from the top of the heap.
// @param: PAD: Amount of free space to leave untrimmed at the top of the heap.
// @return: 1: Memory was released and returned to the kernel.
// @return: 0: No memory could be released.
extern int malloc_trim __P((__size_t __pad));


//////////////////////////////////////////////////////////////////////////
// Returns the amount of usable (that is read/writable)
// memory size, as previously allocated by a call to
// malloc and friends returning the given 'MALLPTR' pointer.
//  - Any pointer accepted by free() is also accepted by this function.
//  - The behavior is undefined for any non-NULL, invalid pointer.
//  - ZERO(0) is returned for a MALLPTR equal to NULL.
// NOTE: Because of internal overallocation, the returned
//       value might be greater than the value originally
//       passed to an allocation function.
// WARNING: In some debug-configurations, the returned size may appear
//          to always be equal to the originally m-allocated size.
//          This only happens when mall is configured to enforce tail
//          validation and is a behavior that should not be relied upon.
extern __wunused __size_t malloc_usable_size __P((void *__restrict __mallptr));


#ifdef __LIBC_HAVE_DEBUG_MALLOC
// Debug versions of malloc functions.
// NOTE: Debug-allocated memory can freely be mixed with non-debug allocated.
// HINT: Memory blocks allocated through non-debug callbacks are
//       not counted towards memory leaks, meaning that you can simply
//       code intended leaks by #undef-ing the malloc functions you're using.
// HINT: Another, even simpler way of addressing the non-debug functions
//       is to put parenthesis around malloc and friends:
//       >> /* Yes: This might look strange, but due to
//       >>  *      the way macros work, this is allowed! */
//       >> void *p = (malloc)(42);
#if !defined(__CONFIG_MIN_LIBC__) || !defined(__LIBC_DEBUG_X_ARGS)
extern __crit __wunused __sizemalloccall((1)) __ATTRIBUTE_ALIGNED_DEFAULT void *_malloc_d __P((__size_t __bytes __LIBC_DEBUG__PARAMS));
extern __crit __wunused __sizemalloccall((1,2)) __ATTRIBUTE_ALIGNED_DEFAULT void *_calloc_d __P((__size_t __count, __size_t __bytes __LIBC_DEBUG__PARAMS));
extern __crit __wunused __sizemalloccall((2)) __nonnull((1)) __ATTRIBUTE_ALIGNED_DEFAULT void *_realloc_d __P((void *__restrict __mallptr, __size_t __bytes __LIBC_DEBUG__PARAMS));
extern __wunused __nonnull((1)) __size_t _malloc_usable_size_d __P((void *__restrict __mallptr __LIBC_DEBUG__PARAMS));
extern __crit __nonnull((1)) void _free_d __P((void *__restrict __freeptr __LIBC_DEBUG__PARAMS));
extern __crit __nonnull((1)) int _posix_memalign_d __P((void **__restrict __memptr, __size_t __alignment, __size_t __bytes __LIBC_DEBUG__PARAMS));
extern __crit __wunused __sizemalloccall((1)) __ATTRIBUTE_ALIGNED_PAGE void *_pvalloc_d __P((__size_t __bytes __LIBC_DEBUG__PARAMS));
extern __crit __wunused __sizemalloccall((1)) __ATTRIBUTE_ALIGNED_PAGE void *_valloc_d __P((__size_t __bytes __LIBC_DEBUG__PARAMS));
extern __crit __wunused __sizemalloccall((2)) __attribute_aligned_a(1) void *_memalign_d __P((__size_t __alignment, __size_t __bytes __LIBC_DEBUG__PARAMS));
#endif
#if !defined(__CONFIG_MIN_LIBC__) || defined(__LIBC_DEBUG_X_ARGS)
extern __crit __wunused __sizemalloccall((1)) __ATTRIBUTE_ALIGNED_DEFAULT void *_malloc_x __P((__size_t __bytes __LIBC_DEBUG_X__PARAMS));
extern __crit __wunused __sizemalloccall((1,2)) __ATTRIBUTE_ALIGNED_DEFAULT void *_calloc_x __P((__size_t __count, __size_t __bytes __LIBC_DEBUG_X__PARAMS));
extern __crit __wunused __sizemalloccall((2)) __nonnull((1)) __ATTRIBUTE_ALIGNED_DEFAULT void *_realloc_x __P((void *__restrict __mallptr, __size_t __bytes __LIBC_DEBUG_X__PARAMS));
extern __wunused __nonnull((1)) __size_t _malloc_usable_size_x __P((void *__restrict __mallptr __LIBC_DEBUG_X__PARAMS));
extern __crit __nonnull((1)) void _free_x __P((void *__restrict __freeptr __LIBC_DEBUG_X__PARAMS));
extern __crit __nonnull((1)) int _posix_memalign_x __P((void **__restrict __memptr, __size_t __alignment, __size_t __bytes __LIBC_DEBUG_X__PARAMS));
extern __crit __wunused __sizemalloccall((1)) __ATTRIBUTE_ALIGNED_PAGE void *_pvalloc_x __P((__size_t __bytes __LIBC_DEBUG_X__PARAMS));
extern __crit __wunused __sizemalloccall((1)) __ATTRIBUTE_ALIGNED_PAGE void *_valloc_x __P((__size_t __bytes __LIBC_DEBUG_X__PARAMS));
extern __crit __wunused __sizemalloccall((2)) __attribute_aligned_a(1) void *_memalign_x __P((__size_t __alignment, __size_t __bytes __LIBC_DEBUG_X__PARAMS));
#endif

//////////////////////////////////////////////////////////////////////////
// Mallblock extension functions
// >> Used for working with/enumerating allocated malloc blocks
// NOTE: When debug-mall is disabled, these are implemented as no-op macros,
//       with the functions still exported as no-op/NULL-return functions.
struct _mallblock_d;

// WARNING: Don't use the following directly. Use the macros below instead.
#define __MALLBLOCK_ATTRIB_FILE  0
#define __MALLBLOCK_ATTRIB_LINE  1
#define __MALLBLOCK_ATTRIB_FUNC  2
#define __MALLBLOCK_ATTRIB_SIZE  3
#define __MALLBLOCK_ATTRIB_ADDR  4
extern __nonnull((1)) void *
__mallblock_getattrib_d __P((struct _mallblock_d const *__restrict __self,
                             int __attrib));

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Query attributes on a given malloc block SELF, as
// can be retrieved when enumerating all allocated blocks.
// WARNING: When debug malloc is disabled, these functions
//          are implement as no-ops always returning NULL/0.
// NOTE: These functions may be (and are) implemented as macros (without any exported prototype)
extern char const *_mallblock_getfile_d(struct _mallblock_d const *__restrict self);
extern int         _mallblock_getline_d(struct _mallblock_d const *__restrict self);
extern char const *_mallblock_getfunc_d(struct _mallblock_d const *__restrict self);
extern __size_t    _mallblock_getsize_d(struct _mallblock_d const *__restrict self);
extern char const *_mallblock_getaddr_d(struct _mallblock_d const *__restrict self);
#else
#define _mallblock_getfile_d(self) ((char const *)__mallblock_getattrib_d(self,__MALLBLOCK_ATTRIB_FILE))
#define _mallblock_getline_d(self) ((int)(__uintptr_t)__mallblock_getattrib_d(self,__MALLBLOCK_ATTRIB_LINE))
#define _mallblock_getfunc_d(self) ((char const *)__mallblock_getattrib_d(self,__MALLBLOCK_ATTRIB_FUNC))
#define _mallblock_getsize_d(self) ((__size_t)__mallblock_getattrib_d(self,__MALLBLOCK_ATTRIB_SIZE))
#define _mallblock_getaddr_d(self) ((void const *)__mallblock_getattrib_d(self,__MALLBLOCK_ATTRIB_ADDR))
#endif

#ifndef __pdebug_stackwalker_defined
#define __pdebug_stackwalker_defined 1
typedef int (*ptbwalker) __P((void const *__restrict __instruction_pointer,
                              void const *__restrict __frame_address,
                              __size_t __frame_index, void *__closure));
#endif

//////////////////////////////////////////////////////////////////////////
// Enumerate the allocation traceback of a given mallblock.
// NOTE: The value passed for 'FRAME_ADDRESS' in 'CALLBACK' is always NULL
// - Usual rules apply, and enumeration can be halted
//   with the same non-ZERO returned by CALLBACK.
extern __nonnull((1,2)) int
_mallblock_traceback_d __P((struct _mallblock_d *__restrict self,
                            ptbwalker __callback,
                            void *__closure));


//////////////////////////////////////////////////////////////////////////
// Enumerate all allocated malloc blocks
// If the given CALLBACK returns non-zero, enumeration aborts with that value.
// >> NOTE: Only one task (thread) may enumerate a given set of blocks at once.
//          If a second task attempts to enumerate mallblocks, whilst another
//          is already doing so, only blocks allocated after the first
//          started to are listed.
//          Similarly, blocks created after enumeration started are not listed either.
//       >> Once enumeration stops, all blocks enumerated can be
//          listed again in a following call to '_malloc_enumblocks_d'.
// >> Blocks are always enumerated by reverse order of allocation,
//    with the latest allocated block enumerated first.
// HINT: For convenience, you can use '_malloc_printleaks_d' to dump
//       a list of all currently allocated blocks, meaning that when
//       called at application shutdown, everything that wasn't freed
//       can be dumped (aka. all the memory leaks).
// @param: CHECKPOINT: Any dynamically allocated pointer (any pointer accepted by 'free()'),
//                     or NULL. When non-NULL, only memory after the given pointer
//                     is enumerated (non-inclusive), whereas when NULL, all
//                     existing mall-blocks are enumerated.
// @return: 0: Enumeration finished without being aborted.
extern __nonnull((2)) int
_malloc_enumblocks_d __P((void *__checkpoint,
                          int (*__callback)(struct _mallblock_d *__restrict __block,
                                            void *__closure),
                          void *__closure));

//////////////////////////////////////////////////////////////////////////
// Dump all allocated mallblocks in a human-readable per-block format including a traceback:
//           | -- repeated 40 times ------------------------- |
// [once] >> ##################################################
// [once] >> {file}({line}) : {func} : Leaked {size} bytes at {addr}
// [many] >> {file}({line}) : {func} : [{frame_index}] : {instruction_addr}
// HINT: libc debug-builds automatically hook this function to be
//       called via 'atexit' when exiting through normal means.
extern void _malloc_printleaks_d __P((void));

//////////////////////////////////////////////////////////////////////////
// Validate the header/footers of all allocated malloc blocks.
// Invalid blocks of memory cause an error to be printed to stderr,
// and the application to be terminated using a failed assertion.
// This function can be called at any time and although highly
// expensive, can be very useful to narrow down the point at
// which some chunk of memory starts being corrupted.
// HINT: Visual C/C++ has an equivalent called '_CrtCheckMemory()'
extern void _malloc_validate_d __P((void));

//////////////////////////////////////////////////////////////////////////
// Untrack a given pointer previously allocated through malloc() and friends.
// When not tracked, the associated allocation cannot be enumerated.
// WARNING: Causes undefined behavior if the given
//          pointer isn't really a mall-pointer.
// NOTE: This function is a no-op if the given MALLPTR wasn't tracked, or if NULL is passed
// @return: * : Always returns MALLPTR.
extern void *__malloc_untrack __P((void *__mallptr));
#if defined(__COMPILER_HAVE_TYPEOF) && (!defined(__INTELLISENSE__) || !defined(__cplusplus))
#   define _malloc_untrack(mallptr) ((__typeof__(mallptr))__malloc_untrack(mallptr))
#elif defined(__cplusplus)
extern "C++" template<class __T> inline __T *_malloc_untrack(__T *__mallptr) { return (__T *)__malloc_untrack((void *)__mallptr); }
#else
#   define _malloc_untrack    __malloc_untrack
#endif

#ifndef __INTELLISENSE__
#ifdef __LIBC_DEBUG_X__ARGS
#define malloc(bytes)                _malloc_x(bytes __LIBC_DEBUG_X__ARGS)
#define calloc(count,bytes)          _calloc_x(count,bytes __LIBC_DEBUG_X__ARGS)
#define realloc(mallptr,bytes)       _realloc_x(mallptr,bytes __LIBC_DEBUG_X__ARGS)
#define memalign(alignment,bytes)    _memalign_x(alignment,bytes __LIBC_DEBUG_X__ARGS)
#define free(mallptr)                _free_x(mallptr __LIBC_DEBUG_X__ARGS)
#define malloc_usable_size(mallptr)  _malloc_usable_size_x(mallptr __LIBC_DEBUG_X__ARGS)
#define posix_memalign(memptr,alignment,bytes) _posix_memalign_x(memptr,alignment,bytes __LIBC_DEBUG_X__ARGS)
#define pvalloc(bytes)               _pvalloc_x(bytes __LIBC_DEBUG_X__ARGS)
#define valloc(bytes)                _valloc_x(bytes __LIBC_DEBUG_X__ARGS)
#else /* __LIBC_DEBUG_X__ARGS */
#define malloc(bytes)                _malloc_d(bytes __LIBC_DEBUG__ARGS)
#define calloc(count,bytes)          _calloc_d(count,bytes __LIBC_DEBUG__ARGS)
#define realloc(mallptr,bytes)       _realloc_d(mallptr,bytes __LIBC_DEBUG__ARGS)
#define memalign(alignment,bytes)    _memalign_d(alignment,bytes __LIBC_DEBUG__ARGS)
#define free(mallptr)                _free_d(mallptr __LIBC_DEBUG__ARGS)
#define malloc_usable_size(mallptr)  _malloc_usable_size_d(mallptr __LIBC_DEBUG__ARGS)
#define posix_memalign(memptr,alignment,bytes) _posix_memalign_d(memptr,alignment,bytes __LIBC_DEBUG__ARGS)
#define pvalloc(bytes)               _pvalloc_d(bytes __LIBC_DEBUG__ARGS)
#define valloc(bytes)                _valloc_d(bytes __LIBC_DEBUG__ARGS)
#endif /* !__LIBC_DEBUG_X__ARGS */
#ifndef __KERNEL__
#define cfree                        free
#endif /* !__KERNEL__ */
#endif /* !__INTELLISENSE__ */
#else /* __LIBC_HAVE_DEBUG_MALLOC */
// mallblock extension function stubs
struct _mallblock_d;
#define _mallblock_getfile_d(self)                    (char const *)0
#define _mallblock_getline_d(self)                    0
#define _mallblock_getfunc_d(self)                    (char const *)0
#define _mallblock_getsize_d(self)                    (__size_t)0
#define _mallblock_getaddr_d(self)                    (void const *)0
#define _mallblock_traceback_d(self,callback,closure) 0
#define _malloc_enumblocks_d(callback,closure)        0
#define _malloc_printleaks_d()                        (void)0
#define _malloc_validate_d()                          (void)0
#define __malloc_untrack(mallptr)            ((void *)(mallptr))
#define _malloc_untrack(mallptr)                      (mallptr)
#endif /* !__LIBC_HAVE_DEBUG_MALLOC */

#ifndef __CONFIG_MIN_LIBC__

//////////////////////////////////////////////////////////////////////////
// Malloc Info extensions (Not available in kernel mode)

#ifdef __BUILDING_LIBC__
#define STRUCT_MALLINFO_DECLARED 1 /*< Keep dlmalloc from defining this structure. */
#endif

#ifndef _STRUCT_MALLINFO
#define _STRUCT_MALLINFO 1
struct mallinfo {
    __size_t arena;    /*< Non-mmapped space allocated (bytes). */
    __size_t ordblks;  /*< Number of free chunks. */
    __size_t smblks;   /*< Number of free fastbin blocks. */
    __size_t hblks;    /*< Number of mmapped regions. */
    __size_t hblkhd;   /*< Space allocated in mmapped regions (bytes). */
    __size_t usmblks;  /*< Maximum total allocated space (bytes). */
    __size_t fsmblks;  /*< Space in freed fastbin blocks (bytes). */
    __size_t uordblks; /*< Total allocated space (bytes). */
    __size_t fordblks; /*< Total free space (bytes). */
    __size_t keepcost; /*< Top-most, releasable space (bytes). */
};
#endif /* !_STRUCT_MALLINFO */

extern struct mallinfo mallinfo __P((void));
extern void malloc_stats __P((void));

#endif /* !__CONFIG_MIN_LIBC__ */

#undef __ATTRIBUTE_ALIGNED_DEFAULT
#undef __ATTRIBUTE_ALIGNED_PAGE

__DECL_END

#ifdef __COMPILER_HAVE_PRAGMA_PUSH_MACRO
/* Restore user-defined malloc macros */
#ifdef __malloc_h_calloc_defined
#undef __malloc_h_calloc_defined
#pragma pop_macro("calloc")
#endif
#ifdef __malloc_h_free_defined
#undef __malloc_h_free_defined
#pragma pop_macro("free")
#endif
#ifdef __malloc_h_malloc_defined
#undef __malloc_h_malloc_defined
#pragma pop_macro("malloc")
#endif
#ifdef __malloc_h_malloc_usable_size_defined
#undef __malloc_h_malloc_usable_size_defined
#pragma pop_macro("malloc_usable_size")
#endif
#ifdef __malloc_h_mallopt_defined
#undef __malloc_h_mallopt_defined
#pragma pop_macro("mallopt")
#endif
#ifdef __malloc_h_realloc_defined
#undef __malloc_h_realloc_defined
#pragma pop_macro("realloc")
#endif
#ifdef __malloc_h_cfree_defined
#undef __malloc_h_cfree_defined
#pragma pop_macro("cfree")
#endif
#ifdef __malloc_h_memalign_defined
#undef __malloc_h_memalign_defined
#pragma pop_macro("memalign")
#endif
#ifdef __malloc_h_posix_memalign_defined
#undef __malloc_h_posix_memalign_defined
#pragma pop_macro("posix_memalign")
#endif
#ifdef __malloc_h_aligned_alloc_defined
#undef __malloc_h_aligned_alloc_defined
#pragma pop_macro("aligned_alloc")
#endif
#ifdef __malloc_h_pvalloc_defined
#undef __malloc_h_pvalloc_defined
#pragma pop_macro("pvalloc")
#endif
#ifdef __malloc_h_palloc_defined
#undef __malloc_h_palloc_defined
#pragma pop_macro("valloc")
#endif
#endif
#endif /* !__ASSEMBLY__ */

#endif /* !_MALLOC_H */
#endif /* !__MALLOC_H__ */
