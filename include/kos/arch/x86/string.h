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
#include <kos/kernel/types.h>

__DECL_BEGIN

__local int __karch_ffs(int i) {
 int result = 0,temp;
 __asm__("bsf %2,%1\n"
         "jz 1f\n"
         "lea 1(%1),%0\n"
         "1:"
         : "=&a" (result), "=r" (temp)
         : "rm" (i));
 return result;
}

__local void *__karch_memcpy(void *__dst, void const *__src, __size_t __bytes) {
 void *__result = __dst;
 __asm_volatile__("cld\n"
                  "rep movsb\n"
                  : "+c" (__bytes)
                  ,"+S" (__src)
                  ,"+D" (__dst)
                  : : "memory");
 return __result;
}
__local void *__karch_memset(void *__dst, int __byte, __size_t __bytes) {
 void *__result = __dst;
 __asm_volatile__("cld\n"
                  "rep stosb\n"
                  : "+c" (__bytes)
                  , "+D" (__dst)
                  : "a" (__byte)
                  : "memory");
 return __result;
}
__local char *__karch_strend(char const *__s) {
 char *__result;
 __asm_volatile__("movl %1, %%edi\n"   /* Store the search string in EDI. */
                  "sub %%al, %%al\n"   /* We're searching for \0. */
                  "sub %%ecx, %%ecx\n" /* No limit when searching. */
                  "not %%ecx\n"        /* ditto (ECX == 0xffffffff). */
                  "cld\n"              /* clear direction flag. */
                  "repne scasb\n"      /* perform the string operation. */
                  "dec %%edi\n"        /* The last searched was the match. - Remove it. */
                  "movl %%edi, %0\n"   /* Store the result. */
                  : "=g" (__result)
                  : "g" (__s)
                  : "memory"); 
 return __result;
}
__local char *__karch_strnend(char const *__s, __size_t __maxchars) {
 char *__result;
 __asm_volatile__("movl %1, %%edi\n"    /* Store the search string in EDI. */
                  "sub %%al, %%al\n"    /* We're searching for \0. */
                  "movl %2, %%ecx\n"    /* Limit search to the given 'n'. */
                  "cld\n"               /* clear direction flag. */
                  "repne scasb\n"       /* perform the string operation. */
                  "jnz 1f\n"            /* If we didn't find it, don't decrement the result. */
                  "dec %%edi\n"         /* Adjust the counter to really point to the end of the string. */
                  "1: movl %%edi, %0\n" /* Store the result. */
                  : "=g" (__result)
                  : "g" (__s)
                  , "g" (__maxchars)
                  : "memory");
 return __result;
}

__local __size_t __karch_strlen(char const *__s) {
 __size_t __result;
 __asm_volatile__("movl %1, %%edi\n"   /* Store the search string in EDI. */
                  "sub %%al, %%al\n"   /* We're searching for \0. */
                  "sub %%ecx, %%ecx\n" /* No limit when searching. */
                  "not %%ecx\n"        /* ditto (ECX == 0xffffffff). */
                  "cld\n"              /* clear direction flag. */
                  "repne scasb\n"      /* perform the string operation. */
                  "not %%ecx\n"        /* ECX now contains the amount of searched characters. */
                  "dec %%ecx\n"        /* The last searched was the match. - Remove it. */
                  "movl %%ecx, %0\n"   /* Store the result. */
                  : "=g" (__result)
                  : "g" (__s)
                  : "memory");
 return __result;
}

__local __size_t __karch_strnlen(char const *__s, __size_t __maxchars) {
 __size_t __result;
 __asm_volatile__("movl %1, %%edi\n"       /* Store the search string in EDI. */
                  "sub %%al, %%al\n"       /* We're searching for \0. */
                  "movl %2, %%edx\n"       /* Store the given 'n' is EDX (re-used later). */
                  "movl %%edx, %%ecx\n"    /* Limit search to the given 'n' (copy from EDX). */
                  "cld\n"                  /* clear direction flag. */
                  "repne scasb\n"          /* perform the string operation. */
                  "jnz 1f\n"               /* If we didn't find it, don't decrement the result. */
                  "incl %%ecx\n"           /* Adjust the counter to really point to the end of the string. */
                  "1: subl %%ecx, %%edx\n" /* Subtract what we searched from EDX. */
                  "movl %%edx, %0\n"       /* Store EDX in the given result. */
                  : "=g" (__result)
                  : "g" (__s)
                  , "g" (__maxchars)
                  : "memory");
 return __result;
}

#define __karch_raw_ffs      __karch_ffs
#define __karch_raw_memcpy   __karch_memcpy
#define __karch_raw_memset   __karch_memset
#define __karch_raw_strend   __karch_strend
#define __karch_raw_strnend  __karch_strnend
#define __karch_raw_strlen   __karch_strlen
#define __karch_raw_strnlen  __karch_strnlen

__DECL_END

#include <kos/arch/generic/string.h>

#endif /* !__KOS_ARCH_X86_STRING_H__ */
