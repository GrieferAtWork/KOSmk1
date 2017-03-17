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
#ifndef __KOS_ARCH_X86_STRING_H__
#define __KOS_ARCH_X86_STRING_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

__local int __karch_ffs(int i) {
 register int result = 0,temp;
 __asm__("bsf %2,%1\n"
         "jz 1f\n"
         "lea 1(%1),%0\n"
         "1:"
         : "=&a" (result), "=r" (temp)
         : "rm" (i));
 return result;
}


#define __karch_memcpy_X(length,dst,src,bytes) \
 __asm_volatile__("cld\n"\
                  "rep movs" length "\n"\
                  : "+c" (bytes)\
                  , "+S" (src)\
                  , "+D" (dst)\
                  : : "memory")

__local void *__karch_memcpy_b(void *__restrict __dst, void const *__restrict __src, __size_t __bytes)  { void *__result = __dst; __karch_memcpy_X("b",__dst,__src,__bytes);  return __result; }
__local void *__karch_memcpy_w(void *__restrict __dst, void const *__restrict __src, __size_t __words)  { void *__result = __dst; __karch_memcpy_X("w",__dst,__src,__words);  return __result; }
__local void *__karch_memcpy_l(void *__restrict __dst, void const *__restrict __src, __size_t __dwords) { void *__result = __dst; __karch_memcpy_X("l",__dst,__src,__dwords); return __result; }
#ifdef __x86_64__
__local void *__karch_memcpy_q(void *__restrict __dst, void const *__restrict __src, __size_t __qwords) { void *__result = __dst; __karch_memcpy_X("q",__dst,__src,__qwords); return __result; }
#endif /* __x86_64__ */
#undef __karch_memcpy_X

#define __karch_memset_X(length,dst,byte,bytes) \
 __asm_volatile__("cld\n"\
                  "rep stos" length "\n"\
                  : "+c" (bytes)\
                  , "+D" (dst)\
                  : "a" (byte)\
                  : "memory")

__local void *__karch_memset_b(void *__restrict __dst, __u8  __byte,  __size_t __bytes)  { void *__result = __dst; __karch_memset_X("b",__dst,__byte,__bytes);   return __result; }
__local void *__karch_memset_w(void *__restrict __dst, __u16 __word,  __size_t __words)  { void *__result = __dst; __karch_memset_X("w",__dst,__word,__words);   return __result; }
__local void *__karch_memset_l(void *__restrict __dst, __u32 __dword, __size_t __dwords) { void *__result = __dst; __karch_memset_X("l",__dst,__dword,__dwords); return __result; }
#ifdef __x86_64__
__local void *__karch_memset_q(void *__restrict __dst, __u32 __qword, __size_t __qwords) { void *__result = __dst; __karch_memset_X("q",__dst,__qword,__qwords); return __result; }
#endif /* __x86_64__ */
#undef __karch_memset_X

#define __karch_strend_X(length,s) \
 __asm_volatile__("cld\n"                  /* clear direction flag. */\
                  "repne scas" length "\n" /* perform the string operation. */\
                  : "+D" (s)\
                  : "a" (0)\
                  , "c" (~(__uintptr_t)0)\
                  : "memory")
__local __u8  *__karch_strend_b(__u8  const *__s) { __karch_strend_X("b",__s); return (__u8  *)__s-1; }
__local __u16 *__karch_strend_w(__u16 const *__s) { __karch_strend_X("w",__s); return (__u16 *)__s-1; }
__local __u32 *__karch_strend_l(__u32 const *__s) { __karch_strend_X("l",__s); return (__u32 *)__s-1; }
#ifdef __x86_64__
__local __u64 *__karch_strend_q(__u64 const *__s) { __karch_strend_X("q",__s); return (__u64 *)__s-1; }
#endif
#undef __karch_strend_X

__local __u8 *__karch_strnend_b(__u8 const *__s, __size_t __maxchars) {
 __asm_volatile__("cld\n"            /* clear direction flag. */
                  "repne scasb\n"    /* perform the string operation. */
                  "jnz 1f\n"         /* If we didn't find it, don't decrement the result. */
                  "dec %%edi\n"      /* Adjust the counter to really point to the end of the string. */
                  "1:\n"
                  : "+D" (__s)
                  : "c" (__maxchars), "a" (0)
                  : "memory");
 return (__u8 *)__s;
}
#define __karch_strnend_X(length,size,s,maxchars) \
 __asm_volatile__("cld\n"                  /* clear direction flag. */\
                  "repne scas" length "\n" /* perform the string operation. */\
                  "jnz 1f\n"               /* If we didn't find it, don't decrement the result. */\
                  "sub $" #size " %%edi\n" /* Adjust the counter to really point to the end of the string. */\
                  "1:\n"\
                  : "+D" (s)\
                  : "c" (maxchars), "a" (0)\
                  : "memory")
__local __u16 *__karch_strnend_w(__u16 const *__s, __size_t __maxchars) { __karch_strnend_X("w",2,__s,__maxchars); return (__u16 *)__s; }
__local __u32 *__karch_strnend_l(__u32 const *__s, __size_t __maxchars) { __karch_strnend_X("l",4,__s,__maxchars); return (__u32 *)__s; }
#ifdef __x86_64__
__local __u64 *__karch_strnend_q(__u64 const *__s, __size_t __maxchars) { __karch_strnend_X("q",8,__s,__maxchars); return (__u64 *)__s; }
#endif
#undef __karch_strnend_X

#define __karch_strlen_X(length,s,result) \
 __asm_volatile__("cld\n"\
                  "repne scas" length "\n"\
                  : "=c" (result)\
                  : "D" (s), "a" (0)\
                  , "c" (~(__uintptr_t)0)\
                  : "memory")
__local __size_t __karch_strlen_b(__u8  const *__s) { register __size_t __result; __karch_strlen_X("b",__s,__result); return ~__result-1; }
__local __size_t __karch_strlen_w(__u16 const *__s) { register __size_t __result; __karch_strlen_X("w",__s,__result); return ~__result-1; }
__local __size_t __karch_strlen_l(__u32 const *__s) { register __size_t __result; __karch_strlen_X("l",__s,__result); return ~__result-1; }
#ifdef __x86_64__
__local __size_t __karch_strlen_q(__u64 const *__s) { register __size_t __result; __karch_strlen_X("q",__s,__result); return ~__result-1; }
#endif
#undef __karch_strlen_X

__local __size_t __karch_strnlen_b(__u8 const *__s, __size_t __maxchars) {
 register __size_t __result = __maxchars;
 __asm_volatile__("cld\n"         /* clear direction flag. */
                  "repne scasb\n" /* perform the string operation. */
                  "jnz 1f\n"      /* If we didn't find it, don't decrement the result. */
                  "inc %%ecx\n"   /* Adjust the counter to really point to the end of the string. */
                  "1:\n"
                  : "+c" (__result)
                  : "D" (__s), "a" (0)
                  : "memory");
 return __maxchars-__result;
}

#define __karch_strnlen_X(length,size,s,result) \
 __asm_volatile__("cld\n"                   /* clear direction flag. */\
                  "repne scas" length "\n"  /* perform the string operation. */\
                  "jnz 1f\n"                /* If we didn't find it, don't decrement the result. */\
                  "add $" #size ", %%ecx\n" /* Adjust the counter to really point to the end of the string. */\
                  "1:\n"\
                  : "+c" (result)\
                  : "D" (s), "a" (0)\
                  : "memory")
__local __size_t __karch_strnlen_w(__u16 const *__s, __size_t __maxchars) { register __size_t __result = __maxchars; __karch_strnlen_X("w",2,__s,__result); return __maxchars-__result; }
__local __size_t __karch_strnlen_l(__u32 const *__s, __size_t __maxchars) { register __size_t __result = __maxchars; __karch_strnlen_X("l",4,__s,__result); return __maxchars-__result; }
#ifdef __x86_64__
__local __size_t __karch_strnlen_q(__u64 const *__s, __size_t __maxchars) { register __size_t __result = __maxchars; __karch_strnlen_X("q",8,__s,__result); return __maxchars-__result; }
#endif
#undef __karch_strnlen_X

__local void *__karch_memchr_b(void const *__p, __u8 __needle, __size_t __bytes) {
 __asm_volatile__("cld\n"              /* clear direction flag. */
                  "repne scasb\n"      /* perform the string operation. */
                  "jz 1f\n"            /* Skip the NULL-return if we did find it. */
                  "xor %%edi, %%edi\n" /* If we didn't find it, return NULL. */
                  "inc %%edi\n"        /* ... */
                  "1: dec %%edi\n"     /* Walk back to the character in question. */
                  : "+D" (__p)
                  : "c" (__bytes)
                  , "a" (__needle)
                  : "memory");
 return (char *)__p;
}

#define __karch_memchr_X(length,s,p,needle,bytes) \
 __asm_volatile__("cld\n"                  /* clear direction flag. */\
                  "repne scas" length "\n" /* perform the string operation. */\
                  "jz 1f\n"                /* Skip the NULL-return if we did find it. */\
                  "xor %%edi, %%edi\n"     /* If we didn't find it, return NULL. */\
                  "add $" #s " %%edi\n"    /* ... */\
                  "1: sub $" #s " %%edi\n" /* Walk back to the character in question. */\
                  : "+D" (p)\
                  : "c" (bytes)\
                  , "a" (needle)\
                  : "memory")
__local void *__karch_memchr_w(void const *__p, __u16 __needle, __size_t __words)  { __karch_memchr_X("w",2,__p,__needle,__words); return (char *)__p; }
__local void *__karch_memchr_l(void const *__p, __u32 __needle, __size_t __dwords) { __karch_memchr_X("l",4,__p,__needle,__dwords); return (char *)__p; }
#ifdef __x86_64__
__local void *__karch_memchr_q(void const *__p, __u64 __needle, __size_t __qwords) { __karch_memchr_X("q",8,__p,__needle,__qwords); return (char *)__p; }
#endif
#undef __karch_memchr_X

__local void *__karch_memrchr_b(void const *__p, __u8 __needle, __size_t __bytes) {
 __p = (void const *)((__uintptr_t)__p+(__bytes-1));
 __asm_volatile__("std\n"                  /* set direction flag. */
                  "repne scasb\n"          /* perform the string operation. */
                  "jz 1f\n"                /* Skip the NULL-return if we did find it. */
                  "xor %%edi, %%edi\n"     /* If we didn't find it, return NULL. */
                  "dec %%edi\n"            /* ... */
                  "1: inc %%edi\n"         /* Walk forward to the character in question. */
                  : "+D" (__p)
                  : "c" (__bytes)
                  , "a" (__needle)
                  : "memory");
 return (void *)__p;
}

#define __karch_memrchr_X(length,s,p,needle,bytes) \
 __asm_volatile__("std\n"                  /* set direction flag. */\
                  "repne scas" length "\n" /* perform the string operation. */\
                  "jz 1f\n"                /* Skip the NULL-return if we did find it. */\
                  "xor %%edi, %%edi\n"     /* If we didn't find it, return NULL. */\
                  "sub $" #s " %%edi\n"    /* ... */\
                  "1: add $" #s " %%edi\n" /* Walk back to the character in question. */\
                  : "+D" (p)\
                  : "c" (bytes)\
                  , "a" (needle)\
                  : "memory")
__local void *__karch_memrchr_w(void const *__p, __u16 __needle, __size_t __words)  { __p = (void const *)((__uintptr_t)__p+((__words-1)*2));  __karch_memrchr_X("w",2,__p,__needle,__words);  return (void *)__p; }
__local void *__karch_memrchr_l(void const *__p, __u32 __needle, __size_t __dwords) { __p = (void const *)((__uintptr_t)__p+((__dwords-1)*4)); __karch_memrchr_X("l",4,__p,__needle,__dwords); return (void *)__p; }
#ifdef __x86_64__
__local void *__karch_memrchr_q(void const *__p, __u64 __needle, __size_t __qwords) { __p = (void const *)((__uintptr_t)__p+((__qwords-1)*8)); __karch_memrchr_X("q",8,__p,__needle,__qwords); return (void *)__p; }
#endif
#undef __karch_memrchr_X

#define __karch_memcmp_X(length,size,ax_name,a,b,bytes,result) \
 __asm_volatile__("cld\n"\
                  "repe cmps" length "\n"\
                  "jz 1f\n"\
                  "mov" length " -" #size "(%%edi), %%" ax_name "\n"\
                  "sub" length " -" #size "(%%esi), %%" ax_name "\n"\
                  "1:\n"\
                  : "+D" (a)\
                  , "+S" (b)\
                  , "+a" (result)\
                  : "c" (bytes)\
                  : "memory")
__local __s8  __karch_memcmp_b(void const *__a, void const *__b, __size_t __bytes)  { register __s8  __result = 0; __karch_memcmp_X("b",1,"al", __a,__b,__bytes, __result); return __result; }
__local __s16 __karch_memcmp_w(void const *__a, void const *__b, __size_t __words)  { register __s16 __result = 0; __karch_memcmp_X("w",2,"ax", __a,__b,__words, __result); return __result; }
__local __s32 __karch_memcmp_l(void const *__a, void const *__b, __size_t __dwords) { register __s32 __result = 0; __karch_memcmp_X("l",4,"eax",__a,__b,__dwords,__result); return __result; }
#ifdef __x86_64__
__local __s64 __karch_memcmp_q(void const *__a, void const *__b, __size_t __qwords) { register __s64 __result = 0; __karch_memcmp_X("q",8,"rax",__a,__b,__qwords,__result); return __result; }
#endif
#undef __karch_memcmp_X


/* TODO: We can implement a _LOT_ more string routines,
 * like (mem|str)chr, (mem|str)cmp.
 * (I think) even something like strstr, and strspn and memmem. */

#define __karch_raw_ffs      __karch_ffs
#define __karch_raw_memcpy   __karch_memcpy_b
#define __karch_raw_memset(dst,byte,bytes) \
 __karch_memset_b(dst,(__u8)(byte),bytes)
#define __karch_raw_memcpy_b __karch_memcpy_b
#define __karch_raw_memcpy_w __karch_memcpy_w
#define __karch_raw_memcpy_l __karch_memcpy_l
#define __karch_raw_memset_b __karch_memset_b
#define __karch_raw_memset_w __karch_memset_w
#define __karch_raw_memset_l __karch_memset_l
#ifdef __x86_64__
#define __karch_raw_memcpy_q __karch_memcpy_q
#define __karch_raw_memset_q __karch_memset_q
#endif /* __x86_64__ */
#define __karch_raw_strend(s)     (char *)__karch_strend_b((__u8 *)(s))
#define __karch_raw_strnend(s,n)  (char *)__karch_strnend_b((__u8 *)(s),n)
#define __karch_raw_strlen(s)     __karch_strlen_b((__u8 *)(s))
#define __karch_raw_strnlen(s,n)  __karch_strnlen_b((__u8 *)(s),n)

#define __karch_raw_memchr(dst,byte,bytes) \
 __karch_memchr_b(dst,(__u8)(byte),bytes)
#define __karch_raw_memchr_b __karch_memchr_b
#define __karch_raw_memchr_w __karch_memchr_w
#define __karch_raw_memchr_l __karch_memchr_l
#ifdef __x86_64__
#define __karch_raw_memchr_q __karch_memchr_q
#endif

#define __karch_raw_memrchr(dst,byte,bytes) \
 __karch_memrchr_b(dst,(__u8)(byte),bytes)
#define __karch_raw_memrchr_b __karch_memrchr_b
#define __karch_raw_memrchr_w __karch_memrchr_w
#define __karch_raw_memrchr_l __karch_memrchr_l
#ifdef __x86_64__
#define __karch_raw_memrchr_q __karch_memrchr_q
#endif

#define __karch_raw_memcmp   __karch_memcmp_b
#define __karch_raw_memcmp_b __karch_memcmp_b
#define __karch_raw_memcmp_w __karch_memcmp_w
#define __karch_raw_memcmp_l __karch_memcmp_l
#ifdef __x86_64__
#define __karch_raw_memcmp_q __karch_memcmp_q
#endif

__DECL_END

/* Autocomplete string functions using generic constant optimizations. */
#include <kos/arch/generic/string.h>

#endif /* !__KOS_ARCH_X86_STRING_H__ */
