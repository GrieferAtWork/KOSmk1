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
#ifndef __KOS_ARCHX86_STRING_H__
#define __KOS_ARCHX86_STRING_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include <kos/types.h>

__DECL_BEGIN

__local int x86_ffs(int i) {
 register int result = 0,temp;
 __asm__("bsf %2,%1\n"
         "jz 1f\n"
         "lea 1(%1),%0\n"
         "1:"
         : "=&a" (result), "=r" (temp)
         : "rm" (i));
 return result;
}


#define x86_memcpyX(length,dst,src,bytes) \
 __asm_volatile__("cld\n"\
                  "rep movs" length "\n"\
                  : "+c" (bytes)\
                  , "+S" (src)\
                  , "+D" (dst)\
                  : : "memory")

__local void *x86_memcpyb(void *__restrict __dst, void const *__restrict __src, __size_t __bytes)  { void *__result = __dst; x86_memcpyX("b",__dst,__src,__bytes);  return __result; }
__local void *x86_memcpyw(void *__restrict __dst, void const *__restrict __src, __size_t __words)  { void *__result = __dst; x86_memcpyX("w",__dst,__src,__words);  return __result; }
__local void *x86_memcpyl(void *__restrict __dst, void const *__restrict __src, __size_t __dwords) { void *__result = __dst; x86_memcpyX("l",__dst,__src,__dwords); return __result; }
#ifdef __x86_64__
__local void *x86_memcpyq(void *__restrict __dst, void const *__restrict __src, __size_t __qwords) { void *__result = __dst; x86_memcpyX("q",__dst,__src,__qwords); return __result; }
#endif /* __x86_64__ */
#undef x86_memcpyX

#define x86_memsetX(length,dst,byte,bytes) \
 __asm_volatile__("cld\n"\
                  "rep stos" length "\n"\
                  : "+c" (bytes)\
                  , "+D" (dst)\
                  : "a" (byte)\
                  : "memory")

__local void *x86_memsetb(void *__restrict __dst, __u8  _byte,  __size_t __bytes)  { void *__result = __dst; x86_memsetX("b",__dst,_byte,__bytes);   return __result; }
__local void *x86_memsetw(void *__restrict __dst, __u16 _word,  __size_t __words)  { void *__result = __dst; x86_memsetX("w",__dst,_word,__words);   return __result; }
__local void *x86_memsetl(void *__restrict __dst, __u32 __dword, __size_t __dwords) { void *__result = __dst; x86_memsetX("l",__dst,__dword,__dwords); return __result; }
#ifdef __x86_64__
__local void *x86_memsetq(void *__restrict __dst, __u32 _qword, __size_t __qwords) { void *__result = __dst; x86_memsetX("q",__dst,_qword,__qwords); return __result; }
#endif /* __x86_64__ */
#undef x86_memsetX

#define x86_strendX(length,s) \
 __asm_volatile__("cld\n"                  /* clear direction flag. */\
                  "repne scas" length "\n" /* perform the string operation. */\
                  : "+D" (s)\
                  : "a" (0)\
                  , "c" (~(__uintptr_t)0)\
                  : "memory")
__local __u8  *x86_strendb(__u8  const *__s) { x86_strendX("b",__s); return (__u8  *)__s-1; }
__local __u16 *x86_strendw(__u16 const *__s) { x86_strendX("w",__s); return (__u16 *)__s-1; }
__local __u32 *x86_strendl(__u32 const *__s) { x86_strendX("l",__s); return (__u32 *)__s-1; }
#ifdef __x86_64__
__local __u64 *x86_strendq(__u64 const *__s) { x86_strendX("q",__s); return (__u64 *)__s-1; }
#endif
#undef x86_strendX

__local __u8 *x86_strnendb(__u8 const *__s, __size_t __maxchars) {
 if __likely(__maxchars) {
  __asm_volatile__("cld\n"            /* clear direction flag. */
                   "repne scasb\n"    /* perform the string operation. */
                   "jnz 1f\n"         /* If we didn't find it, don't decrement the result. */
                   "dec %%edi\n"      /* Adjust the counter to really point to the end of the string. */
                   "1:\n"
                   : "+D" (__s)
                   : "c" (__maxchars), "a" (0)
                   : "memory");
 }
 return (__u8 *)__s;
}
#define x86_strnendX(length,size,s,maxchars) \
 __asm_volatile__("cld\n"                  /* clear direction flag. */\
                  "repne scas" length "\n" /* perform the string operation. */\
                  "jnz 1f\n"               /* If we didn't find it, don't decrement the result. */\
                  "sub $" #size " %%edi\n" /* Adjust the counter to really point to the end of the string. */\
                  "1:\n"\
                  : "+D" (s)\
                  : "c" (maxchars), "a" (0)\
                  : "memory")
__local __u16 *x86_strnendw(__u16 const *__s, __size_t __maxchars) { if __likely(__maxchars) x86_strnendX("w",2,__s,__maxchars); return (__u16 *)__s; }
__local __u32 *x86_strnendl(__u32 const *__s, __size_t __maxchars) { if __likely(__maxchars) x86_strnendX("l",4,__s,__maxchars); return (__u32 *)__s; }
#ifdef __x86_64__
__local __u64 *x86_strnendq(__u64 const *__s, __size_t __maxchars) { if __likely(__maxchars) x86_strnendX("q",8,__s,__maxchars); return (__u64 *)__s; }
#endif
#undef x86_strnendX

#define x86_strlenX(length,s,result) \
 __asm_volatile__("cld\n"\
                  "repne scas" length "\n"\
                  : "=c" (result)\
                  : "D" (s), "a" (0)\
                  , "c" (~(__uintptr_t)0)\
                  : "memory")
__local __size_t x86_strlenb(__u8  const *__s) { register __size_t __result; x86_strlenX("b",__s,__result); return ~__result-1; }
__local __size_t x86_strlenw(__u16 const *__s) { register __size_t __result; x86_strlenX("w",__s,__result); return ~__result-1; }
__local __size_t x86_strlenl(__u32 const *__s) { register __size_t __result; x86_strlenX("l",__s,__result); return ~__result-1; }
#ifdef __x86_64__
__local __size_t x86_strlenq(__u64 const *__s) { register __size_t __result; x86_strlenX("q",__s,__result); return ~__result-1; }
#endif
#undef x86_strlenX

__local __size_t x86_strnlenb(__u8 const *__s, __size_t __maxchars) {
 register __size_t __result;
 if __unlikely(!__maxchars) return 0;
 __asm_volatile__("cld\n"         /* clear direction flag. */
                  "repne scasb\n" /* perform the string operation. */
                  "jnz 1f\n"      /* If we didn't find it, don't decrement the result. */
                  "inc %%ecx\n"   /* Adjust the counter to really point to the end of the string. */
                  "1:\n"
                  : "=c" (__result)
                  : "c" (__maxchars)
                  , "D" (__s), "a" (0)
                  : "memory");
 return __maxchars-__result;
}

#define x86_strnlenX(length,size,s,result,maxchars) \
 __asm_volatile__("cld\n"                   /* clear direction flag. */\
                  "repne scas" length "\n"  /* perform the string operation. */\
                  "jnz 1f\n"                /* If we didn't find it, don't decrement the result. */\
                  "add $" #size ", %%ecx\n" /* Adjust the counter to really point to the end of the string. */\
                  "1:\n"\
                  : "=c" (result)\
                  : "c" (maxchars)\
                  , "D" (s), "a" (0)\
                  : "memory")
__local __size_t x86_strnlenw(__u16 const *__s, __size_t __maxchars) { register __size_t __result; if __unlikely(!__maxchars) return 0; x86_strnlenX("w",2,__s,__result,__maxchars); return __maxchars-__result; }
__local __size_t x86_strnlenl(__u32 const *__s, __size_t __maxchars) { register __size_t __result; if __unlikely(!__maxchars) return 0; x86_strnlenX("l",4,__s,__result,__maxchars); return __maxchars-__result; }
#ifdef __x86_64__
__local __size_t x86_strnlenq(__u64 const *__s, __size_t __maxchars) { register __size_t __result; if __unlikely(!__maxchars) return 0; x86_strnlenX("q",8,__s,__result,__maxchars); return __maxchars-__result; }
#endif
#undef x86_strnlenX

__local void *x86_memchrb(void const *__p, __u8 __needle, __size_t __bytes) {
 if __unlikely(!__bytes) return NULL;
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

#define x86_memchrX(length,s,p,needle,bytes) \
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
__local void *x86_memchrw(void const *__p, __u16 __needle, __size_t __words)  { if __unlikely(!__words)  return NULL; x86_memchrX("w",2,__p,__needle,__words); return (char *)__p; }
__local void *x86_memchrl(void const *__p, __u32 __needle, __size_t __dwords) { if __unlikely(!__dwords) return NULL; x86_memchrX("l",4,__p,__needle,__dwords); return (char *)__p; }
#ifdef __x86_64__
__local void *x86_memchrq(void const *__p, __u64 __needle, __size_t __qwords) { if __unlikely(!__qwords) return NULL; x86_memchrX("q",8,__p,__needle,__qwords); return (char *)__p; }
#endif
#undef x86_memchrX

__local void *x86_memrchrb(void const *__p, __u8 __needle, __size_t __bytes) {
 if __unlikely(!__bytes) return NULL;
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

#define x86_memrchrX(length,s,p,needle,bytes) \
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
__local void *x86_memrchrw(void const *__p, __u16 __needle, __size_t __words)  { if __unlikely(!__words)  return NULL; __p = (void const *)((__uintptr_t)__p+((__words-1)*2));  x86_memrchrX("w",2,__p,__needle,__words);  return (void *)__p; }
__local void *x86_memrchrl(void const *__p, __u32 __needle, __size_t __dwords) { if __unlikely(!__dwords) return NULL; __p = (void const *)((__uintptr_t)__p+((__dwords-1)*4)); x86_memrchrX("l",4,__p,__needle,__dwords); return (void *)__p; }
#ifdef __x86_64__
__local void *x86_memrchrq(void const *__p, __u64 __needle, __size_t __qwords) { if __unlikely(!__qwords) return NULL; __p = (void const *)((__uintptr_t)__p+((__qwords-1)*8)); x86_memrchrX("q",8,__p,__needle,__qwords); return (void *)__p; }
#endif
#undef x86_memrchrX

#define x86_memcmpX(length,size,ax_name,a,b,bytes,result) \
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
__local __s8  x86_memcmpb(void const *__a, void const *_b, __size_t __bytes)  { register __s8  __result = 0; if __likely(__bytes) x86_memcmpX("b",1,"al", __a,_b,__bytes, __result); return __result; }
__local __s16 x86_memcmpw(void const *__a, void const *_b, __size_t __words)  { register __s16 __result = 0; if __likely(__words) x86_memcmpX("w",2,"ax", __a,_b,__words, __result); return __result; }
__local __s32 x86_memcmpl(void const *__a, void const *_b, __size_t __dwords) { register __s32 __result = 0; if __likely(__dwords) x86_memcmpX("l",4,"eax",__a,_b,__dwords,__result); return __result; }
#ifdef __x86_64__
__local __s64 x86_memcmpq(void const *__a, void const *_b, __size_t __qwords) { register __s64 __result = 0; if __likely(__qwords) x86_memcmpX("q",8,"rax",__a,_b,__qwords,__result); return __result; }
#endif
#undef x86_memcmpX


/* TODO: We can implement a _LOT_ more string routines,
 * like (mem|str)chr, (mem|str)cmp.
 * (I think) even something like strstr, and strspn and memmem. */

#define __karch_raw_ffs      x86_ffs
#define __karch_raw_memcpy   x86_memcpyb
#define __karch_raw_memset(dst,byte,bytes) \
 x86_memsetb(dst,(__u8)(byte),bytes)
#define __karch_raw_memcpy_b x86_memcpyb
#define __karch_raw_memcpy_w x86_memcpyw
#define __karch_raw_memcpy_l x86_memcpyl
#define __karch_raw_memset_b x86_memsetb
#define __karch_raw_memset_w x86_memsetw
#define __karch_raw_memset_l x86_memsetl
#ifdef __x86_64__
#define __karch_raw_memcpy_q x86_memcpyq
#define __karch_raw_memset_q x86_memsetq
#endif /* __x86_64__ */
#define __karch_raw_strend(s)     (char *)x86_strendb((__u8 *)(s))
#define __karch_raw_strnend(s,n)  (char *)x86_strnendb((__u8 *)(s),n)
#define __karch_raw_strlen(s)     x86_strlenb((__u8 *)(s))
#define __karch_raw_strnlen(s,n)  x86_strnlenb((__u8 *)(s),n)

#define __karch_raw_memchr(dst,byte,bytes) \
 x86_memchrb(dst,(__u8)(byte),bytes)
#define __karch_raw_memchr_b x86_memchrb
#define __karch_raw_memchr_w x86_memchrw
#define __karch_raw_memchr_l x86_memchrl
#ifdef __x86_64__
#define __karch_raw_memchr_q x86_memchrq
#endif

#define __karch_raw_memrchr(dst,byte,bytes) \
 x86_memrchrb(dst,(__u8)(byte),bytes)
#define __karch_raw_memrchr_b x86_memrchrb
#define __karch_raw_memrchr_w x86_memrchrw
#define __karch_raw_memrchr_l x86_memrchrl
#ifdef __x86_64__
#define __karch_raw_memrchr_q x86_memrchrq
#endif

#define __karch_raw_memcmp   x86_memcmpb
#define __karch_raw_memcmp_b x86_memcmpb
#define __karch_raw_memcmp_w x86_memcmpw
#define __karch_raw_memcmp_l x86_memcmpl
#ifdef __x86_64__
#define __karch_raw_memcmp_q x86_memcmpq
#endif

__DECL_END

/* Autocomplete string functions using generic constant optimizations. */
#include <kos/arch/generic/string.h>
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCHX86_STRING_H__ */
