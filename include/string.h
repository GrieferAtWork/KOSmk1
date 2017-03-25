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
#ifndef __STRING_H__
#ifndef _STRING_H
#ifndef _INC_STRING
#define __STRING_H__ 1
#define _STRING_H 1
#define _INC_STRING 1

#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/__alloca.h>
#include <__null.h>
#include <stdarg.h>
#include <kos/config.h>
#include <kos/types.h>
#ifdef __LIBC_USE_ARCH_OPTIMIZATIONS
#include <kos/arch/string.h>
#endif

#ifdef __COMPILER_HAVE_PRAGMA_PUSH_MACRO
#ifdef strdup
#define __string_h_strdup_defined
#pragma push_macro("strdup")
#endif
#ifdef strndup
#define __string_h_strndup_defined
#pragma push_macro("strndup")
#endif
#ifdef memdup
#define __string_h_memdup_defined
#pragma push_macro("memdup")
#endif
#ifdef _memdup
#define __string_h__memdup_defined
#pragma push_macro("_memdup")
#endif
#ifdef strdupf
#define __string_h_strdupf_defined
#pragma push_macro("strdupf")
#endif
#ifdef vstrdupf
#define __string_h_vstrdupf_defined
#pragma push_macro("vstrdupf")
#endif
#ifdef _strdupf
#define __string_h__strdupf_defined
#pragma push_macro("_strdupf")
#endif
#ifdef _vstrdupf
#define __string_h__vstrdupf_defined
#pragma push_macro("_vstrdupf")
#endif
#endif /* __COMPILER_HAVE_PRAGMA_PUSH_MACRO */

/* Kill user-defined malloc macros. */
#undef strdup
#undef strndup
#undef memdup
#undef _memdup
#undef strdupf
#undef vstrdupf
#undef _strdupf
#undef _vstrdupf

__DECL_BEGIN

#ifndef __size_t_defined
#define __size_t_defined 1
typedef __size_t size_t;
#endif

#if __SIZEOF_POINTER__ == 8
#   define __ATTRIBUTE_ALIGNED_DEFAULT __attribute_aligned_c(16)
#elif __SIZEOF_POINTER__ == 4
#   define __ATTRIBUTE_ALIGNED_DEFAULT __attribute_aligned_c(8)
#elif __SIZEOF_POINTER__ == 2
#   define __ATTRIBUTE_ALIGNED_DEFAULT __attribute_aligned_c(4)
#else
#   define __ATTRIBUTE_ALIGNED_DEFAULT
#endif

#ifndef __STDC_PURE__
#if (!defined(__KERNEL__) || defined(__INTELLISENSE__)) && !defined(__NO_asmname)
extern __crit __wunused __nonnull((1)) __sizemalloccall((2)) __ATTRIBUTE_ALIGNED_DEFAULT void *memdup __P((void const *__p, __size_t __bytes)) __asmname("_memdup");
extern __crit __wunused __nonnull((1)) __attribute_vaformat(__printf__,1,2) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *strdupf __P((char const *__restrict __format, ...)) __asmname("_strdupf");
extern __crit __wunused __nonnull((1)) __attribute_vaformat(__printf__,1,0) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *vstrdupf __P((char const *__restrict __format, va_list __args)) __asmname("_vstrdupf");
#else
#   define memdup   _memdup
#   define strdupf  _strdupf
#   define vstrdupf _vstrdupf
#endif
#endif

#if defined(__i386__) || defined(__x86_64__)
/* >> char *strdupaf(char const *__restrict format, ...);
 * Similar to strdupf, but allocates memory of the stack, instead of the heap.
 * While this function is _very_ useful, be warned that due to the way variadic
 * arguments are managed by cdecl (the only calling convention possible to use
 * for them on most platforms) it is nearly impossible not to waste the stack
 * space that was originally allocated for the arguments (Because in cdecl, the
 * callee is responsible for argument cleanup).
 * String duplicate as fu$k!
 * ANYWAYS: Since its the stack, it shouldn't really matter, but please be advised
 *          that use of these functions fall under the same restrictions as all
 *          other alloca-style functions.
 * >> int open_file_in_folder(char const *folder, char const *file) {
 * >>   return open(strdupaf("%s/%s",folder,file),O_RDONLY);
 * >> }
 */
extern __wunused __retnonnull __malloccall __attribute_vaformat(__printf__,1,2) char *__cdecl _strdupaf(char const *__restrict __fmt, ...);
extern __wunused __retnonnull __malloccall __attribute_vaformat(__printf__,1,0) char *__cdecl _vstrdupaf(char const *__restrict __fmt, va_list args);
#ifdef __INTELLISENSE__
#elif defined(__GNUC__)
/* Dear GCC devs: WHY IS THERE NO '__attribute__((alloca))'?
 * Or better yet! Add something like: '__attribute__((clobber("%esp")))'
 *
 * Here's what the hacky code below does:
 * We must use '__builtin_alloca' to inform the compiler that the stack pointer
 * contract has been broken, meaning that %ESP can (no longer) be used for offsets.
 * NOTE: If you don't believe me that this is required, and think this is just me
 *       ranting about missing GCC functionality, try the following code yourself:
 * >> printf("path = '%s'\n",_strdupaf("%s/%s","/usr","lib")); // OK (Also try cloning this line a bunch of times)
 * >> #undef _strdupaf
 * >> printf("path = '%s'\n",_strdupaf("%s/%s","/usr","lib")); // Breaks
 *
 * NOTE: We also can't do __builtin_alloca(0) because that's optimized away too early
 *       and the compiler will (correctly) not mark %ESP as clobbered internally.
 *       So we're left with no choice but to waste another bit of
 *       stack memory, and more importantly: instructions!
 * Oh and by-the-way: Unlike with str(n)dupa It's only possible to implement
 *                    this as a dedicated function, when wanting to ensure
 *                    one-time evaluation of the variadic arguments.
 *                 -> So we can't just implement the while thing as a macro.
 *                   (OK: '_vstrdupaf' could be, but when are you even going to use any to begin with...)
 * HINT: A standard-compliant, but double-evaluating version would look something like this:
 * >> #define _strdupaf(...) \
 * >>   ({ char *result; size_t s;\
 * >>      s = (snprintf(NULL,0,__VA_ARGS__)+1)*sizeof(char);\
 * >>      result = (char *)__builtin_alloca(s);\
 * >>      snprintf(result,s,__VA_ARGS__);\
 * >>      result;\
 * >>   })
 */
#define _strdupaf(...) \
 __xblock({ char *const __sdares = _strdupaf(__VA_ARGS__);\
            (void)__builtin_alloca(1);\
            __xreturn __sdares;\
 })
#define _vstrdupaf(fmt,args) \
 __xblock({ char *__sdares = _vstrdupaf(fmt,args);\
            (void)__builtin_alloca(1);\
            __xreturn __sdares;\
 })
#else
/* This might not work, because the compiler has no
 * idea these functions are violating the stack layout. */
#endif
#ifndef __STDC_PURE__
/* Define non-stdc-compliant function names. */
#ifdef __INTELLISENSE__
extern __wunused __retnonnull __malloccall __attribute_vaformat(__printf__,1,2) char *strdupaf(char const *__restrict __fmt, ...);
extern __wunused __retnonnull __malloccall __attribute_vaformat(__printf__,1,0) char *vstrdupaf(char const *__restrict __fmt, va_list args);
#else
#   define strdupaf   _strdupaf
#   define vstrdupaf  _vstrdupaf
#endif
#endif
#endif /* ARCH... */


#ifndef __memcpy_defined
#define __memcpy_defined 1
extern __retnonnull __nonnull((1,2)) void *
memcpy __P((void *__restrict __dst,
            void const *__restrict __src,
            __size_t __bytes));
#endif
#ifndef __memccpy_defined
#define __memccpy_defined 1
extern __nonnull((1,2)) void *
memccpy __P((void *__restrict __dst,
             void const *__restrict __src,
             int __c, __size_t __bytes));
#endif
#ifndef __memset_defined
#define __memset_defined 1
extern __retnonnull __nonnull((1)) void *
memset __P((void *__restrict __dst,
            int __byte, __size_t __bytes));
#endif

#ifndef __memmove_defined
#define __memmove_defined 1
extern __retnonnull __nonnull((1,2)) void *
memmove __P((void *__dst, void const *__src, __size_t __bytes));
#endif

#ifndef __memcmp_defined
#define __memcmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int
memcmp __P((void const *__a, void const *__b, __size_t __bytes));
#endif /* !__memcmp_defined */

extern __retnonnull __nonnull((1,2)) char *strcpy __P((char *__dst, char const *__src));
extern __retnonnull __nonnull((1,2)) char *strncpy __P((char *__dst, char const *__src, __size_t __maxchars));

#if !defined(__KERNEL__) || defined(__INTELLISENSE__)
extern __retnonnull __nonnull((1,2)) char *strcat __P((char *__dst, char const *__src));
extern __retnonnull __nonnull((1,2)) char *strncat __P((char *__dst, char const *__src, __size_t __maxchars));
#endif
#ifndef __INTELLISENSE__
#if !defined(__STDC_PURE__) || defined(__KERNEL__)
#   define strcat(dst,src)            strcpy(_strend(dst),src)
#   define strncat(dst,src,maxchars) strncpy(_strend(dst),src,maxchars)
#endif
#endif

#if defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__)
extern "C++" {
/* With function overloading, we can propagate const attributes on arguments in all the right places! ;) */
#ifndef __memchr_defined
#define __memchr_defined 1
extern __wunused __purecall __nonnull((1)) void *memchr(void *__restrict __p, int __needle, size_t __bytes) __asmname("memchr");
extern __wunused __purecall __nonnull((1)) void const *memchr(void const *__restrict __p, int __needle, size_t __bytes) __asmname("memchr");
#endif
#ifndef __memrchr_defined
#define __memrchr_defined 1
extern __wunused __purecall __nonnull((1)) void *memrchr(void *__restrict __p, int __needle, size_t __bytes) __asmname("memrchr");
extern __wunused __purecall __nonnull((1)) void const *memrchr(void const *__restrict __p, int __needle, size_t __bytes) __asmname("memrchr");
#endif
#ifndef ___umemend_defined
#define ___umemend_defined 1
extern __wunused __purecall __nonnull((1)) void *_umemend(void *__restrict __haystack, int __needle) __asmname("_umemend");
extern __wunused __purecall __nonnull((1)) void const *_umemend(void const *__restrict __haystack, int __needle) __asmname("_umemend");
#endif
#ifndef ___memend_defined
#define ___memend_defined 1
extern __wunused __purecall __nonnull((1)) void *_memend(void *__restrict __haystack, int __needle, __size_t __bytes) __asmname("_memend");
extern __wunused __purecall __nonnull((1)) void const *_memend(void const *__restrict __haystack, int __needle, __size_t __bytes) __asmname("_memend");
#endif

#ifndef __memmem_defined
#define __memmem_defined 1
extern __wunused __purecall __nonnull((1,3)) void *memmem(void *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen) __asmname("memmem");
extern __wunused __purecall __nonnull((1,3)) void const *memmem(void const *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen) __asmname("memmem");
#endif /* !__memmem_defined */

#ifndef ___memrmem_defined
#define ___memrmem_defined 1
extern __wunused __purecall __nonnull((1,3)) void *_memrmem(void *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen) __asmname("_memrmem");
extern __wunused __purecall __nonnull((1,3)) void const *_memrmem(void const *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen) __asmname("_memrmem");
#endif /* !___memrmem_defined */

#ifndef ___memidx_defined
#define ___memidx_defined 1
/* In a vector of 'ELEMCOUNT' elements, each aligned by 'ELEMALIGN' and sized 'ELEMSIZE',
 * search for one equal to 'PATTERN' and return a pointer to it, or NULL if it doesn't exist. */
extern __wunused __purecall __nonnull((1,3)) void *_memidx(void *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memidx");
extern __wunused __purecall __nonnull((1,3)) void const *_memidx(void const *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memidx");
#endif /* !___memidx_defined */
#if !defined(__INTELLISENSE__) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS) && defined(arch_memidx)
template<class __T> __wunused __purecall __nonnull((1,3)) __T *_memidx(__T *__vector, __size_t __elemcount, __T const &__pattern) { return (__T *)arch_memidx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T> __wunused __purecall __nonnull((1,3)) __T const *_memidx(__T const *__vector, __size_t __elemcount, __T const &__pattern) { return (__T const *)arch_memidx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T *_memidx(__T (&__vector)[__S], __T const &__pattern) { return (__T *)arch_memidx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T const *_memidx(__T (&__vector)[__S], __T const &__pattern) { return (__T const *)arch_memidx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
#else
template<class __T> __wunused __purecall __nonnull((1,3)) __T *_memidx(__T *__vector, __size_t __elemcount, __T const &__pattern) { return (__T *)_memidx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T> __wunused __purecall __nonnull((1,3)) __T const *_memidx(__T const *__vector, __size_t __elemcount, __T const &__pattern) { return (__T const *)_memidx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T *_memidx(__T (&__vector)[__S], __T const &__pattern) { return (__T *)_memidx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T const *_memidx(__T (&__vector)[__S], __T const &__pattern) { return (__T const *)_memidx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
#endif

#ifndef ___memridx_defined
#define ___memridx_defined 1
/* In a vector of 'ELEMCOUNT' elements, each aligned by 'ELEMALIGN' and sized 'ELEMSIZE',
 * search for one equal to 'PATTERN' and return a pointer to it, or NULL if it doesn't exist. */
extern __wunused __purecall __nonnull((1,3)) void *_memridx(void *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memridx");
extern __wunused __purecall __nonnull((1,3)) void const *_memridx(void const *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memridx");
#endif /* !___memridx_defined */
#if !defined(__INTELLISENSE__) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS) && defined(arch_memridx)
template<class __T> __wunused __purecall __nonnull((1,3)) __T *_memridx(__T *__vector, __size_t __elemcount, __T const &__pattern) { return (__T *)arch_memridx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T> __wunused __purecall __nonnull((1,3)) __T const *_memridx(__T const *__vector, __size_t __elemcount, __T const &__pattern) { return (__T const *)arch_memridx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T *_memridx(__T (&__vector)[__S], __T const &__pattern) { return (__T *)arch_memridx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T const *_memridx(__T (&__vector)[__S], __T const &__pattern) { return (__T const *)arch_memridx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
#else
template<class __T> __wunused __purecall __nonnull((1,3)) __T *_memridx(__T *__vector, __size_t __elemcount, __T const &__pattern) { return (__T *)_memridx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T> __wunused __purecall __nonnull((1,3)) __T const *_memridx(__T const *__vector, __size_t __elemcount, __T const &__pattern) { return (__T const *)_memridx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T *_memridx(__T (&__vector)[__S], __T const &__pattern) { return (__T *)_memridx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T const *_memridx(__T (&__vector)[__S], __T const &__pattern) { return (__T const *)_memridx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
#endif

#ifndef __strchr_defined
#define __strchr_defined 1
extern __wunused __purecall __nonnull((1)) char *strchr(char *__restrict __haystack, int __needle) __asmname("strchr");
extern __wunused __purecall __nonnull((1)) char const *strchr(char const *__restrict __haystack, int __needle) __asmname("strchr");
#endif /* !__strchr_defined */
#ifndef __strrchr_defined
#define __strrchr_defined 1
extern __wunused __purecall __nonnull((1)) char *strrchr(char *__restrict __haystack, int __needle) __asmname("strrchr");
extern __wunused __purecall __nonnull((1)) char const *strrchr(char const *__restrict __haystack, int __needle) __asmname("strrchr");
#endif /* !__strrchr_defined */
extern __wunused __purecall __nonnull((1)) char *_strnchr(char *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strnchr");
extern __wunused __purecall __nonnull((1)) char const *_strnchr(char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strnchr");
extern __wunused __purecall __nonnull((1)) char *_strnrchr(char *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strnrchr");
extern __wunused __purecall __nonnull((1)) char const *_strnrchr(char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strnrchr");

extern __wunused __purecall __nonnull((1,2)) char *strstr(char *__haystack, char const *__needle) __asmname("strstr");
extern __wunused __purecall __nonnull((1,2)) char const *strstr(char const *__haystack, char const *__needle) __asmname("strstr");
extern __wunused __purecall __nonnull((1,2)) char *_strrstr(char *__haystack, char const *__needle) __asmname("_strrstr");
extern __wunused __purecall __nonnull((1,2)) char const *_strrstr(char const *__haystack, char const *__needle) __asmname("_strrstr");
extern __wunused __purecall __nonnull((1,3)) char *_strnstr(char *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strnstr");
extern __wunused __purecall __nonnull((1,3)) char const *_strnstr(char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strnstr");
extern __wunused __purecall __nonnull((1,3)) char *_strnrstr(char *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strnrstr");
extern __wunused __purecall __nonnull((1,3)) char const *_strnrstr(char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strnrstr");

extern __wunused __purecall __nonnull((1,2)) char *strpbrk(char *__haystack, char const *__needle_list) __asmname("strpbrk");
extern __wunused __purecall __nonnull((1,2)) char const *strpbrk(char const *__haystack, char const *__needle_list) __asmname("strpbrk");
extern __wunused __purecall __nonnull((1,2)) char *_strrpbrk(char *__haystack, char const *__needle_list) __asmname("_strrpbrk");
extern __wunused __purecall __nonnull((1,2)) char const *_strrpbrk(char const *__haystack, char const *__needle_list) __asmname("_strrpbrk");
extern __wunused __purecall __nonnull((1,3)) char *_strnbrk(char *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strnpbrk");
extern __wunused __purecall __nonnull((1,3)) char const *_strnbrk(char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strnpbrk");
extern __wunused __purecall __nonnull((1,3)) char *_strnrpbrk(char *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strnrpbrk");
extern __wunused __purecall __nonnull((1,3)) char const *_strnrpbrk(char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strnrpbrk");

/* KOS-extension: Returns a pointer to the ZERO-character terminating a string, or one equal to 'S+MAXCHARS'.
 * This function is equal to, but may be faster than 'S+strlen(S)' / 'S+strnlen(S,MAXCHARS)'. */
extern __wunused __purecall __retnonnull __nonnull((1)) char *_strend(char *__restrict __s) __asmname("_strend");
extern __wunused __purecall __retnonnull __nonnull((1)) char const *_strend(char const *__restrict __s) __asmname("_strend");
extern __wunused __purecall __retnonnull __nonnull((1)) char *_strnend(char *__restrict __s, __size_t __maxchars) __asmname("_strnend");
extern __wunused __purecall __retnonnull __nonnull((1)) char const *_strnend(char const *__restrict __s, __size_t __maxchars) __asmname("_strnend");
#ifndef __STDC_PURE__
/* In a vector of 'ELEMCOUNT' elements, each aligned by 'ELEMALIGN' and sized 'ELEMSIZE',
 * search for one equal to 'PATTERN' and return a pointer to it, or NULL if it doesn't exist. */
extern __wunused __purecall __nonnull((1,3)) void *memidx(void *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memidx");
extern __wunused __purecall __nonnull((1,3)) void const *memidx(void const *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memidx");
extern __wunused __purecall __nonnull((1,3)) void *memridx(void *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memridx");
extern __wunused __purecall __nonnull((1,3)) void const *memridx(void const *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memridx");
#if !defined(__INTELLISENSE__) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS) && defined(arch_memidx)
template<class __T> __wunused __purecall __nonnull((1,3)) __T *memidx(__T *__vector, __size_t __elemcount, __T const &__pattern) { return (__T *)arch_memidx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T> __wunused __purecall __nonnull((1,3)) __T const *memidx(__T const *__vector, __size_t __elemcount, __T const &__pattern) { return (__T const *)arch_memidx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T *memidx(__T (&__vector)[__S], __T const &__pattern) { return (__T *)arch_memidx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T const *memidx(__T (&__vector)[__S], __T const &__pattern) { return (__T const *)arch_memidx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
#else
template<class __T> __wunused __purecall __nonnull((1,3)) __T *memidx(__T *__vector, __size_t __elemcount, __T const &__pattern) { return (__T *)_memidx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T> __wunused __purecall __nonnull((1,3)) __T const *memidx(__T const *__vector, __size_t __elemcount, __T const &__pattern) { return (__T const *)_memidx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T *memidx(__T (&__vector)[__S], __T const &__pattern) { return (__T *)_memidx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T const *memidx(__T (&__vector)[__S], __T const &__pattern) { return (__T const *)_memidx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
#endif
#if !defined(__INTELLISENSE__) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS) && defined(arch_memridx)
template<class __T> __wunused __purecall __nonnull((1,3)) __T *memridx(__T *__vector, __size_t __elemcount, __T const &__pattern) { return (__T *)arch_memridx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T> __wunused __purecall __nonnull((1,3)) __T const *memridx(__T const *__vector, __size_t __elemcount, __T const &__pattern) { return (__T const *)arch_memridx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T *memridx(__T (&__vector)[__S], __T const &__pattern) { return (__T *)arch_memridx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T const *memridx(__T (&__vector)[__S], __T const &__pattern) { return (__T const *)arch_memridx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
#else
template<class __T> __wunused __purecall __nonnull((1,3)) __T *memridx(__T *__vector, __size_t __elemcount, __T const &__pattern) { return (__T *)_memridx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T> __wunused __purecall __nonnull((1,3)) __T const *memridx(__T const *__vector, __size_t __elemcount, __T const &__pattern) { return (__T const *)_memridx(__vector,__elemcount,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T *memridx(__T (&__vector)[__S], __T const &__pattern) { return (__T *)_memridx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
template<class __T, __size_t __S> __wunused __purecall __nonnull((1,3)) __T const *memridx(__T (&__vector)[__S], __T const &__pattern) { return (__T const *)_memridx(__vector,__S,&__pattern,sizeof(__T),__compiler_alignof(__T)); }
#endif

extern __wunused __purecall __nonnull((1)) void *memend(void *__restrict __haystack, int __needle, __size_t __bytes) __asmname("_memend");
extern __wunused __purecall __nonnull((1)) void const *memend(void const *__restrict __haystack, int __needle, __size_t __bytes) __asmname("_memend");
extern __wunused __purecall __nonnull((1)) void *umemend(void *__restrict __haystack, int __needle) __asmname("_umemend");
extern __wunused __purecall __nonnull((1)) void const *umemend(void const *__restrict __haystack, int __needle) __asmname("_umemend");
extern __wunused __purecall __nonnull((1)) char *strnchr(char *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strnchr");
extern __wunused __purecall __nonnull((1)) char const *strnchr(char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strnchr");
extern __wunused __purecall __nonnull((1)) char *strnrchr(char *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strnrchr");
extern __wunused __purecall __nonnull((1)) char const *strnrchr(char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strnrchr");

extern __wunused __purecall __nonnull((1,2)) char *strrstr(char *__haystack, char const *__needle) __asmname("_strrstr");
extern __wunused __purecall __nonnull((1,2)) char const *strrstr(char const *__haystack, char const *__needle) __asmname("_strrstr");
extern __wunused __purecall __nonnull((1,3)) char *strnstr(char *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strnstr");
extern __wunused __purecall __nonnull((1,3)) char const *strnstr(char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strnstr");
extern __wunused __purecall __nonnull((1,3)) char *strnrstr(char *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strnrstr");
extern __wunused __purecall __nonnull((1,3)) char const *strnrstr(char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strnrstr");

extern __wunused __purecall __nonnull((1,2)) char *strrpbrk(char *__haystack, char const *__needle_list) __asmname("_strrpbrk");
extern __wunused __purecall __nonnull((1,2)) char const *strrpbrk(char const *__haystack, char const *__needle_list) __asmname("_strrpbrk");
extern __wunused __purecall __nonnull((1,3)) char *strnpbrk(char *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strnpbrk");
extern __wunused __purecall __nonnull((1,3)) char const *strnpbrk(char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strnpbrk");
extern __wunused __purecall __nonnull((1,3)) char *strnrpbrk(char *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strnrpbrk");
extern __wunused __purecall __nonnull((1,3)) char const *strnrpbrk(char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strnrpbrk");

extern __wunused __purecall __retnonnull __nonnull((1)) char *strend(char *__restrict __s) __asmname("_strend");
extern __wunused __purecall __retnonnull __nonnull((1)) char const *strend(char const *__restrict __s) __asmname("_strend");
extern __wunused __purecall __retnonnull __nonnull((1)) char *strnend(char *__restrict __s, __size_t __maxchars) __asmname("_strnend");
extern __wunused __purecall __retnonnull __nonnull((1)) char const *strnend(char const *__restrict __s, __size_t __maxchars) __asmname("_strnend");
#endif
}
#else /* C++ */
#ifndef __memchr_defined
#define __memchr_defined 1
extern __wunused __purecall __nonnull((1)) void *
memchr __P((void const *__restrict __p, int __needle, size_t __bytes));
#endif
#ifndef __memrchr_defined
#define __memrchr_defined 1
extern __wunused __purecall __nonnull((1)) void *
memrchr __P((void const *__restrict __p, int __needle, size_t __bytes));
#endif
#ifndef ___umemend_defined
#define ___umemend_defined 1
extern __wunused __retnonnull __purecall __nonnull((1)) void *
_umemend __P((void const *__restrict __haystack, int __needle));
#endif
#ifndef ___memend_defined
#define ___memend_defined 1
extern __wunused __retnonnull __purecall __nonnull((1)) void *
_memend __P((void const *__restrict __haystack,
             int __needle, __size_t __bytes));
#endif

#ifndef __memmem_defined
#define __memmem_defined 1
extern __wunused __purecall __nonnull((1,3)) void *
memmem __P((void const *__haystack, size_t __haystacklen,
            void const *__needle, size_t __needlelen));
#endif /* !__memmem_defined */

#ifndef ___memrmem_defined
#define ___memrmem_defined 1
extern __wunused __purecall __nonnull((1,3)) void *_memrmem __P((void const *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen));
#endif /* !___memrmem_defined */

#ifndef ___memidx_defined
#define ___memidx_defined 1
/* In a vector of 'ELEMCOUNT' elements, each aligned by 'ELEMALIGN' and sized 'ELEMSIZE',
 * search for one equal to 'PATTERN' and return a pointer to it, or NULL if it doesn't exist. */
extern __wunused __purecall __nonnull((1,3)) void *_memidx __P((void const *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign));
#endif /* !___memidx_defined */
#ifndef ___memridx_defined
#define ___memridx_defined 1
extern __wunused __purecall __nonnull((1,3)) void *_memridx __P((void const *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign));
#endif /* !___memidx_defined */

#ifndef __strchr_defined
#define __strchr_defined 1
extern __wunused __purecall __nonnull((1)) char *strchr __P((char const *__restrict __haystack, int __needle));
#endif /* !__strchr_defined */
#ifndef __strrchr_defined
#define __strrchr_defined 1
extern __wunused __purecall __nonnull((1)) char *strrchr __P((char const *__restrict __haystack, int __needle));
#endif /* !__strrchr_defined */
extern __wunused __purecall __nonnull((1)) char *_strnchr __P((char const *__restrict __haystack, __size_t __max_haychars, int __needle));
extern __wunused __purecall __nonnull((1)) char *_strnrchr __P((char const *__restrict __haystack, __size_t __max_haychars, int __needle));

extern __wunused __purecall __nonnull((1,2)) char *strstr __P((char const *__haystack, char const *__needle));
extern __wunused __purecall __nonnull((1,2)) char *_strrstr __P((char const *__haystack, char const *__needle));
extern __wunused __purecall __nonnull((1,3)) char *_strnstr __P((char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars));
extern __wunused __purecall __nonnull((1,3)) char *_strnrstr __P((char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars));

extern __wunused __purecall __nonnull((1,2)) char *strpbrk __P((char const *__haystack, char const *__needle_list));
extern __wunused __purecall __nonnull((1,2)) char *_strrpbrk __P((char const *__haystack, char const *__needle_list));
extern __wunused __purecall __nonnull((1,3)) char *_strnpbrk __P((char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist));
extern __wunused __purecall __nonnull((1,3)) char *_strnrpbrk __P((char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist));

extern __wunused __purecall __retnonnull __nonnull((1)) char *_strend __P((char const *__restrict __s));
extern __wunused __purecall __retnonnull __nonnull((1)) char *_strnend __P((char const *__restrict __s, __size_t __maxchars));
#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern __wunused __purecall __nonnull((1,3)) void *memidx __P((void const *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign)) __asmname("_memidx");
extern __wunused __purecall __nonnull((1,3)) void *memridx __P((void const *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign)) __asmname("_memridx");
extern __wunused __purecall __nonnull((1)) void *memend __P((void const *__restrict __haystack, int __needle, __size_t __bytes)) __asmname("_memend");
extern __wunused __purecall __nonnull((1)) void *umemend __P((void const *__restrict __haystack, int __needle)) __asmname("_umemend");
extern __wunused __purecall __nonnull((1,3)) void *memrmem __P((void const *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen)) __asmname("_memrmem");
extern __wunused __purecall __nonnull((1)) char *strnchr __P((char const *__restrict __haystack, __size_t __max_haychars, int __needle)) __asmname("_strnchr");
extern __wunused __purecall __nonnull((1)) char *strnrchr __P((char const *__restrict __haystack, __size_t __max_haychars, int __needle)) __asmname("_strnrchr");
extern __wunused __purecall __nonnull((1,2)) char *strrstr __P((char const *__haystack, char const *__needle)) __asmname("_strrstr");
extern __wunused __purecall __nonnull((1,3)) char *strnstr __P((char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars)) __asmname("_strnstr");
extern __wunused __purecall __nonnull((1,3)) char *strnrstr __P((char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars)) __asmname("_strnrstr");
extern __wunused __purecall __nonnull((1,2)) char *strrpbrk __P((char const *__haystack, char const *__needle_list)) __asmname("_strrpbrk");
extern __wunused __purecall __nonnull((1,3)) char *strnpbrk __P((char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist)) __asmname("_strnpbrk");
extern __wunused __purecall __nonnull((1,3)) char *strnrpbrk __P((char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist)) __asmname("_strnrpbrk");
extern __wunused __purecall __retnonnull __nonnull((1)) char *strend __P((char const *__restrict __s)) __asmname("_strend");
extern __wunused __purecall __retnonnull __nonnull((1)) char *strnend __P((char const *__restrict __s, __size_t __maxchars)) __asmname("_strnend");
#else /* !__NO_asmname */
#   define memidx    _memidx
#   define memridx   _memridx
#   define memend    _memend
#   define umemend   _umemend
#   define memrmem   _memrmem
#   define strnchr   _strnchr
#   define strnrchr  _strnrchr
#   define strrstr   _strrstr
#   define strnstr   _strnstr
#   define strnrstr  _strnrstr
#   define strrpbrk  _strrpbrk
#   define strnpbrk  _strnpbrk
#   define strnrpbrk _strnrpbrk
#   define strend    _strend
#   define strnend   _strnend
#endif /* __NO_asmname */
#endif /* !__STDC_PURE__*/
#endif /* C */

#ifndef ___umemlen_defined
#define ___umemlen_defined 1
extern __wunused __purecall __nonnull((1)) __size_t
_umemlen __P((void const *__restrict __haystack, int __needle));
#endif
#ifndef ___memlen_defined
#define ___memlen_defined 1
extern __wunused __purecall __nonnull((1)) __size_t
_memlen __P((void const *__restrict __haystack,
             int __needle, __size_t __bytes));
#endif

#ifndef __STDC_PURE__
#ifndef __umemlen_defined
#define __umemlen_defined 1
#ifndef __NO_asmname
extern __wunused __purecall __nonnull((1)) __size_t
umemlen __P((void const *__restrict __haystack,
             int __needle)) __asmname("_umemlen");
#else
#define umemlen  _umemlen
#endif
#endif /* !__umemlen_defined */
#ifndef __memlen_defined
#define __memlen_defined 1
#ifndef __NO_asmname
extern __wunused __purecall __nonnull((1)) __size_t
memlen __P((void const *__restrict __haystack,
            int __needle, __size_t __bytes))
            __asmname("_memlen");
#else
#define memlen  _memlen
#endif
#endif /* !__memlen_defined */
#endif /* !__STDC_PURE__ */

extern __wunused __purecall __nonnull((1)) __size_t strlen __P((char const *__restrict __s));
extern __wunused __purecall __nonnull((1)) __size_t strnlen __P((char const *__restrict __s, __size_t __maxchars));

extern __wunused __purecall __nonnull((1,2)) int strcmp __P((char const *__a, char const *__b));
extern __wunused __purecall __nonnull((1,2)) int strncmp __P((char const *__a, char const *__b, __size_t __maxchars));

#ifndef __CONFIG_MIN_LIBC__
/* Non-standard wildcard-enabled string comparison functions.
 * Returns ZERO(0) if a given 'STR' matches a wildcard-enabled pattern 'WILDPATTERN'.
 * Returns <0 if the first miss-matching character in 'STR' is lower than its 'WILDPATTERN' counterpart.
 * Returns >0 if the first miss-matching character in 'STR' is greater than its 'WILDPATTERN' counterpart. */
extern __wunused __purecall __nonnull((1,2)) int _strwcmp __P((char const *__restrict __str, char const *__restrict __wildpattern));
extern __wunused __purecall __nonnull((1,2)) int _striwcmp __P((char const *__restrict __str, char const *__restrict __wildpattern));
extern __wunused __purecall __nonnull((1,3)) int _strnwcmp __P((char const *__restrict __str, __size_t __maxstr, char const *__restrict __wildpattern, __size_t __maxpattern));
extern __wunused __purecall __nonnull((1,3)) int _strinwcmp __P((char const *__restrict __str, __size_t __maxstr, char const *__restrict __wildpattern, __size_t __maxpattern));
#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern __wunused __purecall __nonnull((1,2)) int strwcmp __P((char const *__restrict __str, char const *__restrict __wildpattern)) __asmname("_strwcmp");
extern __wunused __purecall __nonnull((1,2)) int striwcmp __P((char const *__restrict __str, char const *__restrict __wildpattern)) __asmname("_striwcmp");
extern __wunused __purecall __nonnull((1,3)) int strnwcmp __P((char const *__restrict __str, __size_t __maxstr, char const *__restrict __wildpattern, __size_t __maxpattern)) __asmname("_strnwcmp");
extern __wunused __purecall __nonnull((1,3)) int strinwcmp __P((char const *__restrict __str, __size_t __maxstr, char const *__restrict __wildpattern, __size_t __maxpattern)) __asmname("_strinwcmp");
#else
#   define strwcmp   _strwcmp
#   define striwcmp  _striwcmp
#   define strnwcmp  _strnwcmp
#   define strinwcmp _strinwcmp
#endif
#endif
#endif /* !__CONFIG_MIN_LIBC__ */

/* Count the amount of leading characters in 'STR', also contained in 'SPANSET':
 * >> result = 0;
 * >> while (strchr(SPANSET,STR[result])) ++result;
 * >> return result;
 */
extern __wunused __purecall __nonnull((1,2)) __size_t strspn __P((char const *__str, char const *__spanset));
extern __wunused __purecall __nonnull((1,3)) __size_t _strnspn __P((char const *__str, __size_t __maxstr, char const *__spanset, __size_t __maxspanset));

/* Count the amount of leading characters in 'STR', _NOT_ contained in 'SPANSET':
 * >> result = 0;
 * >> while (STR[result] && !strchr(SPANSET,STR[result])) ++result;
 * >> return result;
 */
extern __wunused __purecall __nonnull((1,2)) __size_t strcspn __P((char const *__str, char const *__spanset));
extern __wunused __purecall __nonnull((1,3)) __size_t _strncspn __P((char const *__str, __size_t __maxstr, char const *__spanset, __size_t __maxspanset));

#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern __wunused __purecall __nonnull((1,3)) __size_t strnspn __P((char const *__str, __size_t __maxstr, char const *__spanset, __size_t __maxspanset)) __asmname("_strnspn");
extern __wunused __purecall __nonnull((1,3)) __size_t strncspn __P((char const *__str, __size_t __maxstr, char const *__spanset, __size_t __maxspanset)) __asmname("_strncspn");
#else
#   define strnspn  _strnspn
#   define strncspn _strncspn
#endif
#endif /* !__STDC_PURE__ */


/* Equivalent of >> return (char *)memset(DST,CHR,strlen(DST)). */
/* *ditto*       >> return (char *)memset(DST,CHR,strnlen(DST,MAXBYTES)). */
#ifndef __CONFIG_MIN_LIBC__
extern __retnonnull __purecall __nonnull((1)) char *_strset __P((char *__restrict __dst, int __chr));
extern __retnonnull __purecall __nonnull((1)) char *_strnset __P((char *__restrict __dst, int __chr, __size_t __maxlen));

#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern __retnonnull __purecall __nonnull((1)) char *strset __P((char *__restrict __dst, int __chr)) __asmname("_strset");
extern __retnonnull __purecall __nonnull((1)) char *strnset __P((char *__restrict __dst, int __chr, __size_t __maxlen)) __asmname("_strnset");
#else
#   define strset  _strset
#   define strnset _strnset
#endif
#endif
#else /* !__CONFIG_MIN_LIBC__ */
#define _strset(dst,chr) \
 __xblock({ char *const __dst = (dst);\
            __xreturn (char *)memset(__dst,chr,strlen(__dst));\
 })
#define _strnset(dst,chr,maxlen) \
 __xblock({ char *const __dst = (dst);\
            __xreturn (char *)memset(__dst,chr,strnlen(__dst,maxlen));\
 })
#ifndef __STDC_PURE__
#   define strset  _strset
#   define strnset _strnset
#endif
#endif /* __CONFIG_MIN_LIBC__ */

#ifndef __CONFIG_MIN_LIBC__

/* Some non-standard, uncommon functions, as documented here:
 * http://fresh2refresh.com/c-programming/c-strings/
 */


/* Reverse all characters/bytes in a given string/region of memory, then return that string.
 * NOTE: If a NULL-string is given to 'strrev', it will re-return NULL.
 * RETURN: The given string (== __p/__str).
 * WARNING: The given string will be modified.
 * HINT: You can easily use this in conjunction with strdup to duplicate, then reverse a constant string. */
extern __retnonnull __purecall __nonnull((1)) void *_memrev __P((void *__p, __size_t __bytes));
#ifdef __INTELLISENSE__
extern __purecall char *_strrev __P((char *__str));
extern __retnonnull __purecall __nonnull((1)) char *_strnrev __P((char *__str, __size_t __maxlen));
#ifndef __STDC_PURE__
extern __purecall char *strrev __P((char *__str));
extern __retnonnull __purecall __nonnull((1)) char *strnrev __P((char *__str, __size_t __maxlen));
#endif
#else
#   define _strrev(str)         __xblock({ char *const __s = (str); __xreturn __s ? (char *)_memrev(__s,strlen(__s)) : NULL; })
#   define _strnrev(str,maxlen) __xblock({ char *const __s = (str); __xreturn (char *)_memrev(__s,strnlen(__s,maxlen)); })
#ifndef __STDC_PURE__
#   define strrev    _strrev
#   define strnrev   _strnrev
#endif
#endif

/* Convert a given string to lower/upper case, using the tolower/toupper functions from <ctype.h>
 * NOTE: If a NULL-string is given to 'strlwr'/'strupr', it will re-return NULL.
 * RETURN: The given string (== __str).
 */
extern __purecall char *_strlwr __P((char *__str));
extern __purecall char *_strupr __P((char *__str));
extern __retnonnull __purecall __nonnull((1)) char *_strnlwr __P((char *__str, __size_t __maxlen));
extern __retnonnull __purecall __nonnull((1)) char *_strnupr __P((char *__str, __size_t __maxlen));

/* A differently spelled alias for 'stricmp'. */
#ifndef __NO_asmname
extern __wunused __purecall __nonnull((1,2)) int _strcmpi __P((char const *__a, char const *__b)) __asmname("_stricmp");
#else
#   define _strcmpi  _stricmp
#endif

#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern __retnonnull __purecall __nonnull((1)) void *memrev __P((void *__p, __size_t __bytes)) __asmname("_memrev");
extern __retnonnull __purecall __nonnull((1)) char *strlwr __P((char *__str)) __asmname("_strlwr");
extern __retnonnull __purecall __nonnull((1)) char *strupr __P((char *__str)) __asmname("_strupr");
extern __retnonnull __purecall __nonnull((1)) char *strnlwr __P((char *__str, __size_t __maxlen)) __asmname("_strnlwr");
extern __retnonnull __purecall __nonnull((1)) char *strnupr __P((char *__str, __size_t __maxlen)) __asmname("_strnupr");
extern __wunused __purecall __nonnull((1,2)) int strcmpi __P((char const *__a, char const *__b)) __asmname("_stricmp");
#else
#   define memrev   _memrev
#   define strlwr   _strlwr
#   define strupr   _strupr
#   define strnlwr  _strnlwr
#   define strnupr  _strnupr
#   define strcmpi  _stricmp
#endif
#endif

#endif /* !__CONFIG_MIN_LIBC__ */




#ifndef __CONFIG_MIN_LIBC__
/* Case-insensitive string routines. (The 'i' stands for in-sensitive). */
/* WARNING: Some of these functions were introduced by Microsoft, but
            others might nowhere to be found other than in KOS.
            When portability is required, use with caution! */

#ifndef ___stricmp_defined
#define ___stricmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int _stricmp __P((char const *__a, char const *__b));
#endif /* !___stricmp_defined */
#ifndef ___strincmp_defined
#define ___strincmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int _strincmp __P((char const *__a, char const *__b, __size_t __maxchars));
#endif /* !___strincmp_defined */
#ifndef ___memicmp_defined
#define ___memicmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int _memicmp __P((void const *__a, void const *__b, __size_t __bytes));
#endif /* !___memicmp_defined */

#ifndef __STDC_PURE__
#if !defined(__NO_asmname) || defined(__INTELLISENSE__)
extern __wunused __purecall __nonnull((1,2)) int stricmp __P((char const *__a, char const *__b)) __asmname("_stricmp");
extern __wunused __purecall __nonnull((1,2)) int strincmp __P((char const *__a, char const *__b, __size_t __maxchars)) __asmname("_strincmp");
#else /* !__NO_asmname */
#   define stricmp  _stricmp
#   define strincmp _strincmp
#endif /* __NO_asmname */
#ifndef __memicmp_defined
#define __memicmp_defined 1
#if !defined(__NO_asmname) || defined(__INTELLISENSE__)
extern __wunused __purecall __nonnull((1,2)) int memicmp __P((void const *__a, void const *__b, __size_t __bytes)) __asmname("_memicmp");
#else /* !__NO_asmname */
#   define memicmp  _memicmp
#endif /* __NO_asmname */
#endif /* !__memicmp_defined */
#endif /* !__STDC_PURE__ */

#if defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__)
extern "C++" {
extern __wunused __purecall __nonnull((1)) void *_memichr(void *__restrict __p, int __needle, size_t __bytes) __asmname("_memichr");
extern __wunused __purecall __nonnull((1)) void const *_memichr(void const *__restrict __p, int __needle, size_t __bytes) __asmname("_memichr");
extern __wunused __purecall __nonnull((1)) void *_memirchr(void *__restrict __p, int __needle, size_t __bytes) __asmname("_memirchr");
extern __wunused __purecall __nonnull((1)) void const *_memirchr(void const *__restrict __p, int __needle, size_t __bytes) __asmname("_memirchr");

extern __wunused __purecall __nonnull((1)) char *_strichr(char *__restrict __haystack, int __needle) __asmname("_strichr");
extern __wunused __purecall __nonnull((1)) char const *_strichr(char const *__restrict __haystack, int __needle) __asmname("_strichr");
extern __wunused __purecall __nonnull((1)) char *_strirchr(char *__restrict __haystack, int __needle) __asmname("_strirchr");
extern __wunused __purecall __nonnull((1)) char const *_strirchr(char const *__restrict __haystack, int __needle) __asmname("_strirchr");
extern __wunused __purecall __nonnull((1)) char *_strinchr(char *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strinchr");
extern __wunused __purecall __nonnull((1)) char const *_strinchr(char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strinchr");
extern __wunused __purecall __nonnull((1)) char *_strinrchr(char *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strinrchr");
extern __wunused __purecall __nonnull((1)) char const *_strinrchr(char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strinrchr");

extern __wunused __purecall __nonnull((1,2)) char *_stristr(char *__haystack, char const *__needle) __asmname("_stristr");
extern __wunused __purecall __nonnull((1,2)) char const *_stristr(char const *__haystack, char const *__needle) __asmname("_stristr");
extern __wunused __purecall __nonnull((1,2)) char *_strirstr(char *__haystack, char const *__needle) __asmname("_strirstr");
extern __wunused __purecall __nonnull((1,2)) char const *_strirstr(char const *__haystack, char const *__needle) __asmname("_strirstr");
extern __wunused __purecall __nonnull((1,3)) char *_strinstr(char *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strinstr");
extern __wunused __purecall __nonnull((1,3)) char const *_strinstr(char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strinstr");
extern __wunused __purecall __nonnull((1,3)) char *_strinrstr(char *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strinrstr");
extern __wunused __purecall __nonnull((1,3)) char const *_strinrstr(char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strinrstr");

extern __wunused __purecall __nonnull((1,2)) char *_stripbrk(char *__haystack, char const *__needle_list) __asmname("_stripbrk");
extern __wunused __purecall __nonnull((1,2)) char const *_stripbrk(char const *__haystack, char const *__needle_list) __asmname("_stripbrk");
extern __wunused __purecall __nonnull((1,2)) char *_strirpbrk(char *__haystack, char const *__needle_list) __asmname("_strirpbrk");
extern __wunused __purecall __nonnull((1,2)) char const *_strirpbrk(char const *__haystack, char const *__needle_list) __asmname("_strirpbrk");
extern __wunused __purecall __nonnull((1,3)) char *_strinbrk(char *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strinpbrk");
extern __wunused __purecall __nonnull((1,3)) char const *_strinbrk(char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strinpbrk");
extern __wunused __purecall __nonnull((1,3)) char *_strinrpbrk(char *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strinrpbrk");
extern __wunused __purecall __nonnull((1,3)) char const *_strinrpbrk(char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strinrpbrk");
#ifndef __STDC_PURE__
extern __wunused __purecall __nonnull((1)) void *memichr(void *__restrict __p, int __needle, size_t __bytes) __asmname("_memichr");
extern __wunused __purecall __nonnull((1)) void const *memichr(void const *__restrict __p, int __needle, size_t __bytes) __asmname("_memichr");
extern __wunused __purecall __nonnull((1)) void *memirchr(void *__restrict __p, int __needle, size_t __bytes) __asmname("_memirchr");
extern __wunused __purecall __nonnull((1)) void const *memirchr(void const *__restrict __p, int __needle, size_t __bytes) __asmname("_memirchr");

extern __wunused __purecall __nonnull((1)) char *strichr(char *__restrict __haystack, int __needle) __asmname("_strichr");
extern __wunused __purecall __nonnull((1)) char const *strichr(char const *__restrict __haystack, int __needle) __asmname("_strichr");
extern __wunused __purecall __nonnull((1)) char *strirchr(char *__restrict __haystack, int __needle) __asmname("_strirchr");
extern __wunused __purecall __nonnull((1)) char const *strirchr(char const *__restrict __haystack, int __needle) __asmname("_strirchr");
extern __wunused __purecall __nonnull((1)) char *strinchr(char *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strinchr");
extern __wunused __purecall __nonnull((1)) char const *strinchr(char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strinchr");
extern __wunused __purecall __nonnull((1)) char *strinrchr(char *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strinrchr");
extern __wunused __purecall __nonnull((1)) char const *strinrchr(char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asmname("_strinrchr");

extern __wunused __purecall __nonnull((1,2)) char *stristr(char *__haystack, char const *__needle) __asmname("_stristr");
extern __wunused __purecall __nonnull((1,2)) char const *stristr(char const *__haystack, char const *__needle) __asmname("_stristr");
extern __wunused __purecall __nonnull((1,2)) char *strirstr(char *__haystack, char const *__needle) __asmname("_strirstr");
extern __wunused __purecall __nonnull((1,2)) char const *strirstr(char const *__haystack, char const *__needle) __asmname("_strirstr");
extern __wunused __purecall __nonnull((1,3)) char *strinstr(char *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strinstr");
extern __wunused __purecall __nonnull((1,3)) char const *strinstr(char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strinstr");
extern __wunused __purecall __nonnull((1,3)) char *strinrstr(char *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strinrstr");
extern __wunused __purecall __nonnull((1,3)) char const *strinrstr(char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asmname("_strinrstr");

extern __wunused __purecall __nonnull((1,2)) char *stripbrk(char *__haystack, char const *__needle_list) __asmname("_stripbrk");
extern __wunused __purecall __nonnull((1,2)) char const *stripbrk(char const *__haystack, char const *__needle_list) __asmname("_stripbrk");
extern __wunused __purecall __nonnull((1,2)) char *strirpbrk(char *__haystack, char const *__needle_list) __asmname("_strirpbrk");
extern __wunused __purecall __nonnull((1,2)) char const *strirpbrk(char const *__haystack, char const *__needle_list) __asmname("_strirpbrk");
extern __wunused __purecall __nonnull((1,3)) char *strinbrk(char *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strinpbrk");
extern __wunused __purecall __nonnull((1,3)) char const *strinbrk(char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strinpbrk");
extern __wunused __purecall __nonnull((1,3)) char *strinrpbrk(char *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strinrpbrk");
extern __wunused __purecall __nonnull((1,3)) char const *strinrpbrk(char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asmname("_strinrpbrk");
#endif /* !__STDC_PURE__ */
}
#else /* C++ */
extern __wunused __purecall __nonnull((1)) void *_memichr __P((void const *__restrict __p, int __needle, size_t __bytes));
extern __wunused __purecall __nonnull((1)) void *_memirchr __P((void const *__restrict __p, int __needle, size_t __bytes));

extern __wunused __purecall __nonnull((1)) char *_strichr __P((char const *__restrict __haystack, int __needle));
extern __wunused __purecall __nonnull((1)) char *_strirchr __P((char const *__restrict __haystack, int __needle));
extern __wunused __purecall __nonnull((1)) char *_strinchr __P((char const *__restrict __haystack, __size_t __max_haychars, int __needle));
extern __wunused __purecall __nonnull((1)) char *_strinrchr __P((char const *__restrict __haystack, __size_t __max_haychars, int __needle));

extern __wunused __purecall __nonnull((1,2)) char *_stristr __P((char const *__haystack, char const *__needle));
extern __wunused __purecall __nonnull((1,2)) char *_strirstr __P((char const *__haystack, char const *__needle));
extern __wunused __purecall __nonnull((1,3)) char *_strinstr __P((char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars));
extern __wunused __purecall __nonnull((1,3)) char *_strinrstr __P((char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars));

extern __wunused __purecall __nonnull((1,2)) char *_stripbrk __P((char const *__haystack, char const *__needle_list));
extern __wunused __purecall __nonnull((1,2)) char *_strirpbrk __P((char const *__haystack, char const *__needle_list));
extern __wunused __purecall __nonnull((1,3)) char *_strinpbrk __P((char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist));
extern __wunused __purecall __nonnull((1,3)) char *_strinrpbrk __P((char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist));

#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern __wunused __purecall __nonnull((1)) void *memichr __P((void const *__restrict __p, int __needle, size_t __bytes)) __asmname("_memichr");
extern __wunused __purecall __nonnull((1)) void *memirchr __P((void const *__restrict __p, int __needle, size_t __bytes)) __asmname("_memirchr");

extern __wunused __purecall __nonnull((1)) char *strichr __P((char const *__restrict __haystack, int __needle)) __asmname("_strichr");
extern __wunused __purecall __nonnull((1)) char *strirchr __P((char const *__restrict __haystack, int __needle)) __asmname("_strirchr");
extern __wunused __purecall __nonnull((1)) char *strinchr __P((char const *__restrict __haystack, __size_t __max_haychars, int __needle)) __asmname("_strinchr");
extern __wunused __purecall __nonnull((1)) char *strinrchr __P((char const *__restrict __haystack, __size_t __max_haychars, int __needle)) __asmname("_strinrchr");

extern __wunused __purecall __nonnull((1,2)) char *stristr __P((char const *__haystack, char const *__needle)) __asmname("_stristr");
extern __wunused __purecall __nonnull((1,2)) char *strirstr __P((char const *__haystack, char const *__needle)) __asmname("_strirstr");
extern __wunused __purecall __nonnull((1,3)) char *strinstr __P((char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars)) __asmname("_strinstr");
extern __wunused __purecall __nonnull((1,3)) char *strinrstr __P((char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars)) __asmname("_strinrstr");

extern __wunused __purecall __nonnull((1,2)) char *stripbrk __P((char const *__haystack, char const *__needle_list)) __asmname("_stripbrk");
extern __wunused __purecall __nonnull((1,2)) char *strirpbrk __P((char const *__haystack, char const *__needle_list)) __asmname("_strirpbrk");
extern __wunused __purecall __nonnull((1,3)) char *strinpbrk __P((char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist)) __asmname("_strinpbrk");
extern __wunused __purecall __nonnull((1,3)) char *strinrpbrk __P((char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist)) __asmname("_strinrpbrk");
#else /* !__NO_asmname */
#   define memichr    _memichr
#   define memirchr   _memirchr
#   define strichr    _strichr
#   define strirchr   _strirchr
#   define stristr    _stristr
#   define strirstr   _strirstr
#   define strinchr   _strinchr
#   define strinrchr  _strinrchr
#   define strinstr   _strinstr
#   define strinrstr  _strinrstr
#   define stripbrk   _stripbrk
#   define strirpbrk  _strirpbrk
#   define strinpbrk  _strinpbrk
#   define strinrpbrk _strinrpbrk
#endif /* __NO_asmname */
#endif /* !__STDC_PURE__*/
#endif /* C */
#endif /* !__CONFIG_MIN_LIBC__ */

//////////////////////////////////////////////////////////////////////////
// Strtok (String tokenization)
extern __nonnull((2,3)) char *strtok_r __P((char *__restrict __str, const char *__restrict __delim, char **__restrict __saveptr));
extern __nonnull((2,4)) char *_strntok_r __P((char *__restrict __str, const char *__restrict __delim, __size_t __delim_max, char **__restrict __saveptr));
#ifndef __CONFIG_MIN_LIBC__
extern __nonnull((2,3)) char *_stritok_r __P((char *__restrict __str, const char *__restrict __delim, char **__restrict __saveptr));
extern __nonnull((2,4)) char *_strintok_r __P((char *__restrict __str, const char *__restrict __delim, __size_t __delim_max, char **__restrict __saveptr));
#endif /* __CONFIG_MIN_LIBC__ */

#ifndef __CONFIG_MIN_BSS__
extern __nonnull((2)) char *strtok __P((char *__restrict __str, const char *__restrict __delim));
#endif /* !__CONFIG_MIN_BSS__ */


//////////////////////////////////////////////////////////////////////////
// Duplicate a given string/memory.
// NOTE: The returned pointer must be freed with 'free'
// >> KOS provides an extension for quickly duplicating
//    blocks of memory through the use of 'memdup'.
//////////////////////////////////////////////////////////////////////////
// >> void *data; size_t size;
// >>
// >> acquire_lock(&lock);
// >> size = protected_data_size;
// >> data = memdup(protected_data,
// >>               protected_data_size);
// >> release_lock(&lock);
// >>
// >> if (data) process_data(data,size);
// >>
// >> free(data);
//////////////////////////////////////////////////////////////////////////
extern __crit __wunused __nonnull((1)) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *strdup __P((char const *__restrict __s));
extern __crit __wunused __nonnull((1)) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *strndup __P((char const *__restrict __s, __size_t __maxchars));
extern __crit __wunused __nonnull((1)) __sizemalloccall((2)) __ATTRIBUTE_ALIGNED_DEFAULT void *_memdup __P((void const *__restrict __p, __size_t __bytes));

//////////////////////////////////////////////////////////////////////////
// Duplicate a given string, but also expand printf-style format specifiers:
// >> thread->name = strdupf("debug_thread %d",get_thread_num());
// Just like anything else, use 'free()' to cleanup such a string.
extern __crit __wunused __nonnull((1)) __attribute_vaformat(__printf__,1,2) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_strdupf __P((char const *__restrict __format, ...));
extern __crit __wunused __nonnull((1)) __attribute_vaformat(__printf__,1,0) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_vstrdupf __P((char const *__restrict __format, va_list __args));


#ifdef __LIBC_HAVE_DEBUG_MALLOC
#if !defined(__CONFIG_MIN_LIBC__) || !defined(__LIBC_DEBUG_X_ARGS)
extern __crit __wunused __nonnull((1)) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_strdup_d __P((char const *__restrict __s __LIBC_DEBUG__PARAMS));
extern __crit __wunused __nonnull((1)) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_strndup_d __P((char const *__restrict __s, __size_t __maxchars __LIBC_DEBUG__PARAMS));
extern __crit __wunused __nonnull((1)) __sizemalloccall((2)) __ATTRIBUTE_ALIGNED_DEFAULT void *_memdup_d __P((void const *__restrict __p, __size_t __bytes __LIBC_DEBUG__PARAMS));
extern __crit __wunused __nonnull((1)) __attribute_vaformat(__printf__,1,2) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_strdupf_d __P((__LIBC_DEBUG_PARAMS_ char const *__restrict __format, ...));
extern __crit __wunused __nonnull((1)) __attribute_vaformat(__printf__,1,0) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_vstrdupf_d __P((char const *__restrict __format, va_list __args __LIBC_DEBUG__PARAMS));
#endif
#if !defined(__CONFIG_MIN_LIBC__) || defined(__LIBC_DEBUG_X_ARGS)
extern __crit __wunused __nonnull((1)) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_strdup_x __P((char const *__restrict __s __LIBC_DEBUG_X__PARAMS));
extern __crit __wunused __nonnull((1)) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_strndup_x __P((char const *__restrict __s, __size_t __maxchars __LIBC_DEBUG_X__PARAMS));
extern __crit __wunused __nonnull((1)) __sizemalloccall((2)) __ATTRIBUTE_ALIGNED_DEFAULT void *_memdup_x __P((void const *__restrict __p, __size_t __bytes __LIBC_DEBUG_X__PARAMS));
extern __crit __wunused __nonnull((1)) __attribute_vaformat(__printf__,1,2) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_strdupf_x __P((__LIBC_DEBUG_X_PARAMS_ char const *__restrict __format, ...));
extern __crit __wunused __nonnull((1)) __attribute_vaformat(__printf__,1,0) __malloccall __ATTRIBUTE_ALIGNED_DEFAULT char *_vstrdupf_x __P((char const *__restrict __format, va_list __args __LIBC_DEBUG_X__PARAMS));
#endif

#ifndef __INTELLISENSE__
#   undef strdup
#   undef strndup
#   undef _memdup
#   undef _strdupf
#   undef _vstrdupf
#ifdef __LIBC_DEBUG_X__ARGS
#   define strdup(s)                _strdup_x(s __LIBC_DEBUG_X__ARGS)
#   define strndup(s,maxchars)     _strndup_x(s,maxchars __LIBC_DEBUG_X__ARGS)
#   define _memdup(p,bytes)         _memdup_x(p,bytes __LIBC_DEBUG_X__ARGS)
#   define _strdupf(...)           _strdupf_x(__LIBC_DEBUG_X_ARGS_ __VA_ARGS__)
#   define _vstrdupf(format,args) _vstrdupf_x(format,args __LIBC_DEBUG_X__ARGS)
#else
#   define strdup(s)                _strdup_d(s __LIBC_DEBUG__ARGS)
#   define strndup(s,maxchars)     _strndup_d(s,maxchars __LIBC_DEBUG__ARGS)
#   define _memdup(p,bytes)         _memdup_d(p,bytes __LIBC_DEBUG__ARGS)
#   define _strdupf(...)           _strdupf_d(__LIBC_DEBUG_ARGS_ __VA_ARGS__)
#   define _vstrdupf(format,args) _vstrdupf_d(format,args __LIBC_DEBUG__ARGS)
#endif
#ifndef __STDC_PURE__
#   undef memdup
#   undef strdupf
#   undef vstrdupf
#   define memdup   _memdup
#   define strdupf  _strdupf
#   define vstrdupf _vstrdupf
#endif /* ... */
#endif /* !__INTELLISENSE__ */
#elif defined(__OPTIMIZE__) && !defined(__INTELLISENSE__)
#undef strdup
#define strdup(s) \
 __xblock({ char const *const __sds = (s);\
            __xreturn _memdup(__sds,(strlen(__sds)+1)*sizeof(char));\
 })
#endif


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// alloca-style mem/strdup functions
extern __malloccall char *strdupa(char const *__restrict __s);
extern __malloccall char *strndupa(char const *__restrict __s, __size_t __maxchars);
extern __sizemalloccall((2)) void *_memdupa(void const *__restrict __p, __size_t __bytes);
#ifndef __STDC_PURE__
extern __sizemalloccall((2)) void *memdupa(void const *__restrict __p, __size_t __bytes);
#endif
#else
#define _memdupa(p,bytes) \
 __xblock({ __size_t const __mabts = (bytes);\
            void *const __mares = __alloca(__mabts);\
            __xreturn memcpy(__mares,p,__mabts);\
 })
#define strdupa(s) \
 __xblock({ char const *const __sas = (s);\
            __xreturn (char *)_memdupa(__sas,(strlen(__sas)+1)*sizeof(char));\
 })
#define strndupa(s,maxchars) \
 __xblock({ char const *const __snas = (s);\
            __size_t const __snacc = strnlen(__snas,maxchars);\
            char *const __snar = (char *)__alloca((__snacc+1)*sizeof(char));\
            memcpy(__snar,__snas,__snacc);\
            __snar[__snacc] = '\0';\
            __xreturn __snar;\
 })
#ifndef __STDC_PURE__
#   define memdupa _memdupa
#endif
#endif

#ifdef __ALLOCA_H__
#ifdef __INTELLISENSE__
extern void *_calloca(__size_t count, __size_t size);
extern void *calloca(__size_t count, __size_t size);
#else
#define _calloca(count,size) \
 __xblock({ __size_t const __s = (count)*(size);\
            void *const __res = __alloca(__s);\
            __xreturn memset(__res,0,__s);\
 })
#ifndef __STDC_PURE__
#   define calloca   _calloca
#endif
#endif
#   define _ocalloca(T)   ((T *)_calloca(1,sizeof(T)))
#ifndef __STDC_PURE__
#   define ocalloca  _ocalloca
#endif
#endif /* __ALLOCA_H__ */

#ifndef __CONFIG_MIN_BSS__
extern char *strerror __P((int __eno));
extern char *strerror_r __P((int __eno, char *__restrict __buf, __size_t __buflen));
#endif


#ifdef arch_ffs
#   define _ffs8   arch_ffs
#   define _ffs16  arch_ffs
#   define _ffs32  arch_ffs
#   define _ffs64  arch_ffs
#   define _ffs    arch_ffs
#   undef  ffs
#   define ffs     arch_ffs
#   define ffsl    arch_ffs
#   define ffsll   arch_ffs
#elif !defined(__STRING_C__)
#ifndef __ffsx_defined
#define __ffsx_defined 1
extern __wunused __constcall int _ffs8  __P((__u8 __i));
extern __wunused __constcall int _ffs16 __P((__u16 __i));
extern __wunused __constcall int _ffs32 __P((__u32 __i));
extern __wunused __constcall int _ffs64 __P((__u64 __i));
#endif /* !__ffsx_defined */
#ifdef __CONFIG_MIN_LIBC__
#ifndef __NO_asmname
#ifndef __ffs_defined
#define __ffs_defined 1
extern __wunused __constcall int ffs __P((int __i))
 __asmname(__PP_STR(__PP_CAT_2(_ffs,__PP_MUL8(__SIZEOF_INT__))));
#endif /* !__ffs_defined */
extern __wunused __constcall int ffsl __P((long __i))
 __asmname(__PP_STR(__PP_CAT_2(_ffs,__PP_MUL8(__SIZEOF_LONG__))));
#ifndef __NO_longlong
extern __wunused __constcall int ffsll __P((long long __i))
 __asmname(__PP_STR(__PP_CAT_2(_ffs,__PP_MUL8(__SIZEOF_LONG_LONG__))));
#endif /* !__NO_longlong */
#else /* !__NO_asmname */
#undef  ffs
#define ffs    __PP_CAT_2(_ffs,__PP_MUL8(__SIZEOF_INT__))
#define ffsl   __PP_CAT_2(_ffs,__PP_MUL8(__SIZEOF_LONG__))
#ifndef __NO_longlong
#define ffsll  __PP_CAT_2(_ffs,__PP_MUL8(__SIZEOF_LONG_LONG__))
#endif /* !__NO_longlong */
#endif /* __NO_asmname */
#else /* __CONFIG_MIN_LIBC__ */
#ifndef __ffs_defined
#define __ffs_defined 1
extern __wunused __constcall int ffs __P((int __i));
#endif /* !__ffs_defined */
extern __wunused __constcall int ffsl __P((long __i));
#ifndef __NO_longlong
extern __wunused __constcall int ffsll __P((long long __i));
#endif /* !__NO_longlong */
#endif /* !__CONFIG_MIN_LIBC__ */
#endif /* ... */

#ifdef _WIN32
#undef memcpy_s
#   define memcpy_s(dst,dstsize,src,bytes)    memcpy(dst,src,bytes)
#   define strcpy_s(dst,dstsize,src)          strcpy(dst,src)
#   define strcat_s(dst,dstsize,src)          strcat(dst,src)
#   define strncpy_s(dst,dstsize,src,maxlen)  strncpy(dst,src,maxlen)
#   define strncat_s(dst,dstsize,src,maxlen)  strncat(dst,src,maxlen)
#   define strnlen_s(src,srcsize)             strlen(src)
#   define memmove_s(dst,dstsize,src,bytes)   memmove(dst,src,bytes)
#   define _strdup                            strdup
#   define _strset_s(dst,dstsize,chr)         _strset(dst,chr)
#   define _strnset_s(dst,dstsize,chr,maxlen) _strnset(dst,chr,maxlen)
#ifndef __CONFIG_MIN_LIBC__
#   define _strlwr_s(str,strsize)             _strlwr(str)
#   define _strupr_s(str,strsize)             _strupr(str)
#endif /* !__CONFIG_MIN_LIBC__ */
#endif

__DECL_END

#undef __ATTRIBUTE_ALIGNED_DEFAULT

#if !defined(__INTELLISENSE__) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
/* Pull in arch-specific, optimized string operations */
#   define memcpy                arch_memcpy
#   define memmove               arch_memmove
#   define memset                arch_memset
#   define memcmp                arch_memcmp
#   define memchr                arch_memchr
#   define memrchr               arch_memrchr
#   define memmem                arch_memmem
#   define _memrmem              arch_memrmem
#   define _memend               arch_memend
#   define _memlen               arch_memlen
#   define _umemend              arch_umemend
#   define _umemlen              arch_umemlen
#   define _strlen(s)            arch_umemlen(s,'\0')
#   define _strnlen(s,n)         arch_memlen(s,'\0',n)
#   define _strend(s)    (char *)arch_umemend(s,'\0')
#   define _strnend(s,n) (char *)arch_memend(s,'\0',n)
#ifndef __STDC_PURE__
#   define memrmem               arch_memrmem
#   define memend                arch_memend
#   define memlen                arch_memlen
#   define umemend               arch_umemend
#   define umemlen               arch_umemlen
#   define strlen(s)             arch_umemlen(s,'\0')
#   define strnlen(s,n)          arch_memlen(s,'\0',n)
#   define strend(s)     (char *)arch_umemend(s,'\0')
#   define strnend(s,n)  (char *)arch_memend(s,'\0',n)
#endif /* !__STDC_PURE__ */
#if !(defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__))
#   define _memidx               arch_memidx
#   define _memridx              arch_memridx
#ifndef __STDC_PURE__
#   define memidx                arch_memidx
#   define memridx               arch_memridx
#endif /* !__STDC_PURE__ */
#endif /* !CXX... */
#endif /* __LIBC_USE_ARCH_OPTIMIZATIONS */

#if defined(__GNUC__) && !defined(__INTELLISENSE__) && !defined(__STRING_C__)
/* Bring some gcc-specific compiler-optimizations into the game. */
#undef strlen
#undef strend
#undef strnlen
#undef strnend
#define __constant_strlen        __builtin_strlen
#define __constant_strnlen(s,maxlen)  \
 __xblock({ __size_t const __n = (maxlen);\
            __size_t const __mn = __builtin_strlen(s);\
            __xreturn __mn < __n ? __mn : __n;\
 })
#define __constant_strend(s)         ((s)+__constant_strlen(s))
#define __constant_strnend(s,maxlen) ((s)+__constant_strnlen(s,maxlen))
#ifdef __LIBC_USE_ARCH_OPTIMIZATIONS
#   define strlen(s)            (__builtin_constant_p(s) ? __constant_strlen(s) : arch_umemlen(s,'\0'))
#   define strnlen(s,maxlen)    (__builtin_constant_p(s) ? __constant_strnlen(s,maxlen) : arch_memlen(s,'\0',maxlen))
#   define strend(s)            (__builtin_constant_p(s) ? __constant_strend(s) : (char *)arch_umemend(s,'\0'))
#   define strnend(s,maxlen)    (__builtin_constant_p(s) ? __constant_strnend(s) : (char *)arch_memend(s,'\0',maxlen))
#ifndef __arch_generic_memchr
#   define strchr(s,c)          (__builtin_constant_p(s) ? (char *)arch_memchr(s,c,__constant_strlen(s)) : strchr(s,c))
#   define strnchr(s,maxlen,c)  (__builtin_constant_p(s) ? (char *)arch_memchr(s,c,__constant_strnlen(s,maxlen)) : strnchr(s,maxlen,c))
#endif /* !__arch_generic_memchr */
#ifndef __arch_generic_memrchr
#   define strrchr(s,c)         (__builtin_constant_p(s) ? (char *)arch_memrchr(s,c,__constant_strlen(s)) : strrchr(s,c))
#   define strnrchr(s,maxlen,c) (__builtin_constant_p(s) ? (char *)arch_memrchr(s,c,__constant_strnlen(s,maxlen)) : strnrchr(s,maxlen,c))
#endif /* !__arch_generic_memrchr */
#else /* __LIBC_USE_ARCH_OPTIMIZATIONS */
#   define strlen(s)            (__builtin_constant_p(s) ? __constant_strlen(s) : strlen(s))
#   define strnlen(s,maxlen)    (__builtin_constant_p(s) ? __constant_strnlen(s,maxlen) : strnlen(s,maxlen))
#   define strend(s)            (__builtin_constant_p(s) ? __constant_strend(s) : strend(s))
#   define strnend(s,maxlen)    (__builtin_constant_p(s) ? __constant_strnend(s,maxlen) : strnend(s,maxlen))
#endif /* !__LIBC_USE_ARCH_OPTIMIZATIONS */
#endif /* __GNUC__... */

#ifdef __COMPILER_HAVE_PRAGMA_PUSH_MACRO
/* Restore user-defined malloc-macros */
#ifdef __string_h_strndup_defined
#undef __string_h_strndup_defined
#pragma pop_macro("strndup")
#endif
#ifdef __string_h_strdup_defined
#undef __string_h_strdup_defined
#pragma pop_macro("strdup")
#endif
#ifdef __string_h_memdup_defined
#undef __string_h_memdup_defined
#pragma pop_macro("memdup")
#endif
#ifdef __string_h__memdup_defined
#undef __string_h__memdup_defined
#pragma pop_macro("_memdup")
#endif
#ifdef __string_h_strdupf_defined
#undef __string_h_strdupf_defined
#pragma pop_macro("strdupf")
#endif
#ifdef __string_h_vstrdupf_defined
#undef __string_h_vstrdupf_defined
#pragma pop_macro("vstrdupf")
#endif
#ifdef __string_h__strdupf_defined
#undef __string_h__strdupf_defined
#pragma pop_macro("_strdupf")
#endif
#ifdef __string_h__vstrdupf_defined
#undef __string_h__vstrdupf_defined
#pragma pop_macro("_vstrdupf")
#endif
#endif /* __COMPILER_HAVE_PRAGMA_PUSH_MACRO */
#endif /* !__ASSEMBLY__ */

#endif /* !_INC_STRING */
#endif /* !_STRING_H */
#endif /* !__STRING_H__ */
