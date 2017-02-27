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
#ifndef __KOS_CONFIG_H__
#define __KOS_CONFIG_H__ 1

/* Auto-detect debug configuration.
 * Used in many places to dynamicaly determine on debug configurations.
 */
#if defined(__DEBUG__)\
 || defined(_DEBUG) || defined(DEBUG)
#undef NDEBUG
#endif

#if defined(__RELEASE__)\
 || defined(_RELEASE) || defined(RELEASE)
#define NDEBUG 1
#ifndef __OPTIMIZE__
/* Enable optimizations in release-mode. */
#define __OPTIMIZE__ 1
#endif
#endif

#ifndef NDEBUG
#define __DEBUG__ 1
#endif

/* Don't enable any special features when configuring in minimal-mode. */
#ifndef __CONFIG_MIN_LIBC__
#define __KOS_HAVE_SYSCALL
#define __KOS_HAVE_NTSYSCALL
#define __KOS_HAVE_UNIXSYSCALL
#define __LIBC_HAVE_ATEXIT            /*< atexit(), on_exit(). */
#define __LIBC_HAVE_SBRK              /*< Emulate brk/sbrk using mmap/munmap. */
//#define __LIBC_HAVE_SBRK_THREADSAFE /*< Make brk/sbrk be thread-safe (Not required by posix, or dlmalloc). */
#endif

#define __LIBC_HAVE_DEBUG_PARAMS  3
#define __LIBC_DEBUG_UPARAMS      char const *__unused(__file), int __unused(__line), char const *__unused(__func)
#define __LIBC_DEBUG_UPARAMS_     char const *__unused(__file), int __unused(__line), char const *__unused(__func),
#define __LIBC_DEBUG__UPARAMS    ,char const *__unused(__file), int __unused(__line), char const *__unused(__func)
#define __LIBC_DEBUG_NULL         NULL,-1,NULL
#define __LIBC_DEBUG_NULL_        NULL,-1,NULL,
#define __LIBC_DEBUG__NULL       ,NULL,-1,NULL
#ifdef __DEBUG__
#define __LIBC_DEBUG_PARAMS       char const *__file, int __line, char const *__func
#define __LIBC_DEBUG_PARAMS_      char const *__file, int __line, char const *__func,
#define __LIBC_DEBUG__PARAMS     ,char const *__file, int __line, char const *__func
#define __LIBC_DEBUG_ARGS         __FILE__,__LINE__,__compiler_FUNCTION()
#define __LIBC_DEBUG_ARGS_        __FILE__,__LINE__,__compiler_FUNCTION(),
#define __LIBC_DEBUG__ARGS       ,__FILE__,__LINE__,__compiler_FUNCTION()
#define __LIBC_DEBUG_FWD          __file,__line,__func
#define __LIBC_DEBUG_FWD_         __file,__line,__func,
#define __LIBC_DEBUG__FWD        ,__file,__line,__func
#define __LIBC_DEBUG_FILE         __file
#define __LIBC_DEBUG_LINE         __line
#define __LIBC_DEBUG_FUNC         __func
#else
#define __LIBC_DEBUG_PARAMS       __LIBC_DEBUG_UPARAMS
#define __LIBC_DEBUG_PARAMS_      __LIBC_DEBUG_UPARAMS_
#define __LIBC_DEBUG__PARAMS      __LIBC_DEBUG__UPARAMS
#define __LIBC_DEBUG_ARGS         __LIBC_DEBUG_NULL
#define __LIBC_DEBUG_ARGS_        __LIBC_DEBUG_NULL_
#define __LIBC_DEBUG__ARGS        __LIBC_DEBUG__NULL
#define __LIBC_DEBUG_FWD          __LIBC_DEBUG_NULL
#define __LIBC_DEBUG_FWD_         __LIBC_DEBUG_NULL_
#define __LIBC_DEBUG__FWD         __LIBC_DEBUG__NULL
#define __LIBC_DEBUG_FILE         (char const *)0
#define __LIBC_DEBUG_LINE         (int)-1
#define __LIBC_DEBUG_FUNC         (char const *)0
#endif

#ifdef __DEBUG__
#define __LIBC_HAVE_DEBUG_MALLOC  /* malloc(), calloc(), realloc(), free(), strdup() */
#define __LIBC_HAVE_DEBUG_MEMCHECKS
#ifdef __KERNEL__
#define __KERNEL_HAVE_DEBUG_STACKCHECKS
#endif
#endif


#ifndef __LIBC_HAVE_DEBUG_MEMCHECKS
#ifdef __OPTIMIZE__
/* Use arch-specific optimized memory functions, such as strlen, memcpy and others. */
#define __LIBC_USE_ARCH_OPTIMIZATIONS
#endif
#endif


#endif /* !__KOS_CONFIG_H__ */
