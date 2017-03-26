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
#include <__null.h>
#include <kos/types.h>
#include <kos/endian.h>

__DECL_BEGIN

__local int x86_ffs16(__u16 i) {
 register __u16 result = 0,temp;
 __asm__("bsfw %2,%1\n"
         "jz 1f\n"
#if __SIZEOF_POINTER__ == 2
         "lea 1(%1),%0\n"
#else
         "mov %1, %0\n"
         "inc %0\n"
#endif
         "1:"
         : "=&a" (result), "=r" (temp)
         : "rm" (i));
 return (int)result;
}

__local int x86_ffs32(__u32 i) {
 register __u32 result = 0,temp;
 __asm__("bsfl %2,%1\n"
         "jz 1f\n"
#if __SIZEOF_POINTER__ == 4
         "lea 1(%1),%0\n"
#else
         "mov %1, %0\n"
         "inc %0\n"
#endif
         "1:"
         : "=&a" (result), "=r" (temp)
         : "rm" (i));
 return (int)result;
}


#define x86_memcpyX(length,dst,src,bytes) \
 __asm_volatile__("rep movs" length "\n" : \
                  : "c" (bytes)\
                  , "S" (src)\
                  , "D" (dst)\
                  : "memory")
__local void *x86_memcpyb(void *__restrict __dst, void const *__restrict __src, __size_t __bytes)  { x86_memcpyX("b",__dst,__src,__bytes);  return __dst; }
__local void *x86_memcpyw(void *__restrict __dst, void const *__restrict __src, __size_t __words)  { x86_memcpyX("w",__dst,__src,__words);  return __dst; }
__local void *x86_memcpyl(void *__restrict __dst, void const *__restrict __src, __size_t __dwords) { x86_memcpyX("l",__dst,__src,__dwords); return __dst; }
#ifdef __x86_64__
__local void *x86_memcpyq(void *__restrict __dst, void const *__restrict __src, __size_t __qwords) { x86_memcpyX("q",__dst,__src,__qwords); return __dst; }
#endif /* __x86_64__ */
#undef x86_memcpyX

#define x86_memsetX(length,dst,byte,bytes) \
 __asm_volatile__("rep stos" length "\n" : \
                  : "c" (bytes)\
                  , "D" (dst)\
                  , "a" (byte)\
                  : "memory")

__local void *x86_memsetb(void *__restrict __dst, __u8  __byte,  __size_t __bytes)  { x86_memsetX("b",__dst,__byte,__bytes);   return __dst; }
__local void *x86_memsetw(void *__restrict __dst, __u16 __word,  __size_t __words)  { x86_memsetX("w",__dst,__word,__words);   return __dst; }
__local void *x86_memsetl(void *__restrict __dst, __u32 __dword, __size_t __dwords) { x86_memsetX("l",__dst,__dword,__dwords); return __dst; }
#ifdef __x86_64__
__local void *x86_memsetq(void *__restrict __dst, __u32 __qword, __size_t __qwords) { x86_memsetX("q",__dst,__qword,__qwords); return __dst; }
#endif /* __x86_64__ */
#undef x86_memsetX

__local void *x86_memchrb(void const *__p, __u8 __needle, __size_t __bytes) {
 if __unlikely(!__bytes) return NULL;
 __asm_volatile__("repne scasb\n"      /* perform the string operation. */
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
 __asm_volatile__("repne scas" length "\n"  /* perform the string operation. */\
                  "jz 1f\n"                 /* Skip the NULL-return if we did find it. */\
                  "xor %%edi, %%edi\n"      /* If we didn't find it, return NULL. */\
                  "add $" #s ", %%edi\n"    /* ... */\
                  "1: sub $" #s ", %%edi\n" /* Walk back to the character in question. */\
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
 __asm_volatile__("std\n"              /* Set the direction flag. */
                  "repne scasb\n"      /* perform the string operation. */
                  "jz 1f\n"            /* Skip the NULL-return if we did find it. */
                  "xor %%edi, %%edi\n" /* If we didn't find it, return NULL. */
                  "dec %%edi\n"        /* ... */
                  "1: inc %%edi\n"     /* Walk forward to the character in question. */
                  "cld\n"              /* Reset the direction flag. */
                  : "+D" (__p)
                  : "c" (__bytes)
                  , "a" (__needle)
                  : "memory");
 return (void *)__p;
}

#define x86_memrchrX(length,s,p,needle,bytes) \
 __asm_volatile__("std\n"                   /* Set the direction flag. */\
                  "repne scas" length "\n"  /* perform the string operation. */\
                  "jz 1f\n"                 /* Skip the NULL-return if we did find it. */\
                  "xor %%edi, %%edi\n"      /* If we didn't find it, return NULL. */\
                  "sub $" #s ", %%edi\n"    /* ... */\
                  "1: add $" #s ", %%edi\n" /* Walk back to the character in question. */\
                  "cld\n"                   /* Reset the direction flag. */\
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
 __asm_volatile__("repe cmps" length "\n"\
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



#define x86_umemendX(length,haystack,needle) \
 __asm_volatile__("repne scas" length "\n" /* perform the string operation. */\
                  : "+D" (haystack)\
                  : "a" (needle)\
                  , "c" (~(__uintptr_t)0)\
                  : "memory")
__local void *x86_umemendb(void const *__haystack, __u8  __needle) { x86_umemendX("b",__haystack,__needle); return (void *)((__u8  *)__haystack-1); }
__local void *x86_umemendw(void const *__haystack, __u16 __needle) { x86_umemendX("w",__haystack,__needle); return (void *)((__u16 *)__haystack-1); }
__local void *x86_umemendl(void const *__haystack, __u32 __needle) { x86_umemendX("l",__haystack,__needle); return (void *)((__u32 *)__haystack-1); }
#ifdef __x86_64__
__local void *x86_umemendq(void const *__haystack, __u64 __needle) { x86_umemendX("q",__haystack,__needle); return (void *)((__u64 *)__haystack-1); }
#endif
#undef x86_umemendX

#define x86_umemlenX(length,haystack,needle,result) \
 __asm_volatile__("repne scas" length "\n"\
                  : "=c" (result)\
                  : "D" (haystack), "a" (needle)\
                  , "c" (~(__uintptr_t)0)\
                  : "memory")
__local __size_t x86_umemlenb(void const *__haystack, __u8  __needle) { register __size_t __result; x86_umemlenX("b",__haystack,__needle,__result); return ~__result-1; }
__local __size_t x86_umemlenw(void const *__haystack, __u16 __needle) { register __size_t __result; x86_umemlenX("w",__haystack,__needle,__result); return ~__result-1; }
__local __size_t x86_umemlenl(void const *__haystack, __u32 __needle) { register __size_t __result; x86_umemlenX("l",__haystack,__needle,__result); return ~__result-1; }
#ifdef __x86_64__
__local __size_t x86_umemlenq(void const *__haystack, __u64 __needle) { register __size_t __result; x86_umemlenX("q",__haystack,__needle,__result); return ~__result-1; }
#endif
#undef x86_umemlenX

__local void *x86_memendb(void const *__haystack, __u8 __needle, __size_t __maxbytes) {
 if __likely(__maxbytes) {
  __asm_volatile__("repne scasb\n"    /* perform the string operation. */
                   "jnz 1f\n"         /* If we didn't find it, don't decrement the result. */
                   "dec %%edi\n"      /* Adjust the counter to really point to the end of the string. */
                   "1:\n"
                   : "+D" (__haystack)
                   : "c" (__maxbytes)
                   , "a" (__needle)
                   : "memory");
 }
 return (void *)__haystack;
}
#define x86_memendX(length,size,haystack,needle,maxchars) \
 __asm_volatile__("repne scas" length "\n" /* perform the string operation. */\
                  "jnz 1f\n"               /* If we didn't find it, don't decrement the result. */\
                  "sub $" #size " %%edi\n" /* Adjust the counter to really point to the end of the string. */\
                  "1:\n"\
                  : "+D" (haystack)\
                  : "c" (maxchars)\
                  , "a" (needle)\
                  : "memory")
__local void *x86_memendw(void const *__haystack, __u16 __needle, __size_t __maxwords)  { if __likely(__maxwords)  x86_memendX("w",2,__haystack,__needle,__maxwords);  return (__u16 *)__haystack; }
__local void *x86_memendl(void const *__haystack, __u32 __needle, __size_t __maxdwords) { if __likely(__maxdwords) x86_memendX("l",4,__haystack,__needle,__maxdwords); return (__u32 *)__haystack; }
#ifdef __x86_64__
__local void *x86_memendq(void const *__haystack, __u64 __needle, __size_t __maxqwords) { if __likely(__maxqwords) x86_memendX("q",8,__haystack,__needle,__maxqwords); return (__u64 *)__haystack; }
#endif
#undef x86_memendX

__local __size_t x86_memlenb(void const *__haystack, __u8 __needle, __size_t __maxchars) {
 register __size_t __result;
 if __unlikely(!__maxchars) return 0;
 __asm_volatile__("repne scasb\n" /* perform the string operation. */
                  "jnz 1f\n"      /* If we didn't find it, don't decrement the result. */
                  "inc %%ecx\n"   /* Adjust the counter to really point to the end of the string. */
                  "1:\n"
                  : "=c" (__result)
                  : "c" (__maxchars)
                  , "D" (__haystack)
                  , "a" (__needle)
                  : "memory");
 return __maxchars-__result;
}

#define x86_memlenX(length,size,haystack,needle,maxchars,result) \
 __asm_volatile__("repne scas" length "\n"  /* perform the string operation. */\
                  "jnz 1f\n"                /* If we didn't find it, don't decrement the result. */\
                  "add $" #size ", %%ecx\n" /* Adjust the counter to really point to the end of the string. */\
                  "1:\n"\
                  : "=c" (result)\
                  : "c" (maxchars)\
                  , "D" (haystack)\
                  , "a" (needle)\
                  : "memory")
__local __size_t x86_memlenw(__u16 const *__haystack, __u16 __needle, __size_t __maxchars) { register __size_t __result; if __unlikely(!__maxchars) return 0; x86_memlenX("w",2,__haystack,__needle,__maxchars,__result); return __maxchars-__result; }
__local __size_t x86_memlenl(__u32 const *__haystack, __u32 __needle, __size_t __maxchars) { register __size_t __result; if __unlikely(!__maxchars) return 0; x86_memlenX("l",4,__haystack,__needle,__maxchars,__result); return __maxchars-__result; }
#ifdef __x86_64__
__local __size_t x86_memlenq(__u64 const *__haystack, __u64 __needle, __size_t __maxchars) { register __size_t __result; if __unlikely(!__maxchars) return 0; x86_memlenX("q",8,__haystack,__needle,__maxchars,__result); return __maxchars-__result; }
#endif
#undef x86_strnlenX

/* Sum all bytes in BASE..SIZE together, ignoring overflow.
 * >> Useful for calculating checksums. */
__local __byte_t x86_memsum __D2(void const *,__base,size_t,__size) {
 register __u16 __sum = 0;
 __asm__("    shr %%ecx\n"     /* Shift the least significant bit into CF (set if 'size' is uneven) (Set ZF is size is zero). */
         "    jnc 1f\n"        /* 'size' was even (don't need to read 1 byte). */
         "    lodsb\n"         /* Read one byte into EAX (to align 'size' to words) (Doesn't affect flags). */
         "    mov %%al, %b0\n" /* Store the first byte (Doesn't affect flags). */
         "1:  jz 3f\n"         /* If 'size' is 0, skip to the end. */
         "2:  lodsw\n"         /* Load a word from ESI into AX and increment ESI by 2. */
         "    add %%al, %b0\n" /* Add the low byte. */
         "    add %%ah, %h0\n" /* Add the high byte. */
         "    loop 2b\n"       /* Continue until ECX hits zero. */
         "    add %%bh, %b0\n" /* Add the high byte of BX to its lower one. */
         "3:\n"
         : "+r" (__sum)
         : "S" (__base), "c" (__size)
         : "memory", "eax");
 return (__byte_t)__sum;
}


__local __retnonnull __nonnull((1,2)) void *x86_memcpy
__D3(void *__restrict,__dst,void const *__restrict,__src,__size_t,__bytes) {
 /* General-purpose memcpy (Can transfer any size and works very good with large ones) */
 __asm_volatile__("    shr %%ecx\n"
                  "    jnc 1f\n"    /* 'bytes' was even (don't need to move alignment byte). */
                  "    movsb\n"     /* Copy alignment byte ('BYTES' is now properly aligned by 2). */
                  "1:  shr %%ecx\n"
                  "    jnc 2f\n"    /* 'bytes' was even (don't need to move alignment byte). */
                  "    movsw\n"     /* Copy alignment byte ('BYTES' is now properly aligned by 4). */
                  "2:  rep movsl\n" /* Copy the actual data block (transferring 4 bytes at-a-time). */
                  :
                  : "D" (__dst)
                  , "S" (__src)
                  , "c" (__bytes)
                  : "memory");
 return __dst;
}

__local __retnonnull __nonnull((1,2)) void *x86_memmove
__D3(void *,__dst,void const *,__src,__size_t,__bytes) {
 register __u32 __temp;
 /* General-purpose memmove (Can transfer any size and works very good with large ones) */
 __asm_volatile__("    cmp %%esi, %%edi\n" /* Compare the layout of EDI and ESI (NOTE: AT&T has rever operand order). */
                  "    jg 5f\n"            /* DST >  SRC --> Must move memory in reverse. */
#if 1
                  "    je 9f\n"            /* DST == SRC --> No transfer necessary. */
#endif
                                           /* DST <  SRC --> The equivalent of a regular memcpy. */
                  "    shr %%ecx\n"
                  "    jnc 1f\n"    /* 'bytes' was even (don't need to move alignment byte). */
                  "    movsb\n"     /* Copy alignment byte ('BYTES' is now properly aligned by 2). */
                  "1:  shr %%ecx\n"
                  "    jnc 2f\n"    /* 'bytes' was even (don't need to move alignment byte). */
                  "    movsw\n"     /* Copy alignment byte ('BYTES' is now properly aligned by 4). */
                  "2:  rep movsl\n"
                  "    jmp 9f\n"

                  "5:  std\n" /* DST(EDI) > SRC(ESI) */
                  "    add %%ecx, %%edi\n" /* Move the DST and SRC pointer to the back. */
                  "    add %%ecx, %%esi\n" /* *ditto*. */
                  "    shr %%ecx\n"
                  "    jnc 6f\n"    /* 'bytes' was even (don't need to move alignment byte). */
                  "    dec %%edi\n" /* Adjust to point to the last relevant byte. */
                  "    dec %%esi\n" /* *ditto*. */
                  "    movb (%%esi), %b0\n" /* Copy alignment byte ('BYTES' is now properly aligned by 2). */
                  "    movb %b0, (%%edi)\n"     
                  "6:  shr %%ecx\n"
                  "    jnc 7f\n"    /* 'bytes' was even (don't need to move alignment byte). */
                  "    sub $2, %%edi\n"
                  "    sub $2, %%esi\n"
                  "    movw (%%esi), %w0\n" /* Copy alignment word ('BYTES' is now properly aligned by 4). */
                  "    movw %w0, (%%edi)\n"     
                  "7:  sub $4, %%edi\n"
                  "    sub $4, %%esi\n"
                  "    rep movsl\n"
                  "    cld\n"
                  "9:\n"
                  : "=&r" (__temp)
                  : "D" (__dst)
                  , "S" (__src)
                  , "c" (__bytes)
                  : "memory");
 return __dst;
}

__local __retnonnull __nonnull((1)) void *x86_memset4
__D3(void *__restrict,__dst,__u32,__dword,__size_t,__bytes) {
 /* General-purpose memset (with 4-byte codeword) */
 __asm_volatile__("    shr %%ecx\n"
                  "    jnc 1f\n"    /* 'bytes' was even (don't need to move alignment byte). */
                  "    stosb\n"     /* Fill alignment byte ('BYTES' is now properly aligned by 2). */
                  "1:  shr %%ecx\n"
                  "    jnc 2f\n"    /* 'bytes' was even (don't need to move alignment byte). */
                  "    stosw\n"     /* Copy alignment byte ('BYTES' is now properly aligned by 4). */
                  "2:  rep stosl\n" /* Copy the actual data block (transferring 4 bytes at-a-time). */
                  :
                  : "D" (__dst)
                  , "a" (__dword)
                  , "c" (__bytes)
                  : "memory");
 return __dst;
}

__local __retnonnull __nonnull((1)) void *x86_memset
__D3(void *__restrict,__dst,int,__byte,__size_t,__bytes) {
 /* General-purpose memset (Can fill any size and works very good with large ones) */
 register __u16 __temp;
 __asm_volatile__("    shr %%ecx\n"
                  "    jnc 1f\n"          /* 'bytes' was even (don't need to move alignment byte). */
                  "    stosb\n"           /* Fill alignment byte ('BYTES' is now properly aligned by 2). */
                  "1:  movb %%al, %%ah\n" /* Copy 0x00ff into 0xff00 in the fill register. */
                  "    shr %%ecx\n"
                  "    jnc 2f\n"          /* 'bytes' was even (don't need to move alignment byte). */
                  "    stosw\n"           /* Copy alignment byte ('BYTES' is now properly aligned by 4). */
                  "2:  mov %%ax, %0\n"    /* Copy 0x0000ffff into 0xffff0000 in the fill register. */
                  "    shl $16, %%eax\n"  /* ... */
                  "    mov %0, %%ax\n"    /* ... */
                  "    rep stosl\n"       /* Copy the actual data block (transferring 4 bytes at-a-time). */
                  : "=&r" (__temp)
                  : "D" (__dst)
                  , "a" (__byte)
                  , "c" (__bytes)
                  : "memory");
 return __dst;
}

__local __wunused __nonnull((1,2)) int x86_memcmp
__D3(void const *__restrict,__a,void const *__restrict,__b,__size_t,__bytes) {
 register int __result = 0;
 /* General-purpose memcmp (Most effective for large quantities of memory). */
 __asm_volatile__("    shr %%ecx\n" /* Align by 2. */
                  "    jnc 1f\n"    /* BYTES was already aligned by 2. */
                  "    cmpsb\n"     /* Compare 1 byte for 2-byte alignment offset. */
                  "    jne 11f\n"   /* First byte mismatch. */
                  "1:  shr %%ecx\n" /* Align by 4. */
                  "    jnc 2f\n"    /* BYTES was already aligned by 4. */
                  "    cmpsw\n"     /* Compare 2 bytes for 4-byte alignment offset. */
                  "    jne 12f\n"   /* First word mismatch. */
                  "2:  rep cmpsl\n" /* Iterate the entire memory region and stop if a miss-match is found. */
                  "    je 9f\n"     /* The entire memory region is equal.
                                     * NOTE: This checks the ZF flag, which has two purposes:
                                     * When 'BYTES' < 4, 'rep' didn't modify EFLAGS, but ZF would have already been
                                     * set by one of the prior 'SHR' or 'cmps' instructions, meaning that it being
                                     * set at this point either means that the memory regions are equal, or that
                                     * the given BYTES < 4 and the first 0..BYTES..3 bytes are equal, or BYTES is 0.
                                     * >> All of these cases lead us to return 0. */
                  "14: movl -4(%%edi), %k0\n" /* [LONG] Calculate offset of first miss-matched byte. */
                  "    subl -4(%%esi), %k0\n"
#if __BYTE_ORDER == __LITTLE_ENDIAN
                  "24: testl $0xffff0000, %k0\n" /* if (EAX & 0xffff0000) EAX >>= 16; */
                  "    jz 22f\n"                 /* Skip right-shift of 0xffff0000. */
                  "    shr $16, %k0\n"           /* EAX >>= 16; */
#elif __BYTE_ORDER == __BIG_ENDIAN
                  "24: testw $0x0000ffff, %k0\n" /* if (EAX & 0x0000ffff) EAX <<= 16; */
                  "    jz 22f\n"                 /* Skip left-shift of 0x0000ffff. */
                  "    shl $16, %k0\n"           /* EAX <<= 16; */
#else
#   error "Invalid endian"
#endif
                  "    jmp 22f\n" /* Scan the lower-order word. */
                  "12: movw -2(%%edi), %w0\n" /* [WORD] Calculate offset of first miss-matched byte. */
                  "    subw -2(%%esi), %w0\n"
#if __BYTE_ORDER == __LITTLE_ENDIAN
                  "22: testw $0xff00, %w0\n"  /* if (AX & 0xff00) AX >>= 8; */
                  "    jz 21f\n"              /* Skip right-shift of 0xff00. */
                  "    shr $8, %%ax\n"        /* AX >>= 8; */
#elif __BYTE_ORDER == __BIG_ENDIAN
                  "22: testw $0x00ff, %w0\n"  /* if (AX & 0x00ff) AX <<= 8; */
                  "    jz 21f\n"              /* Skip left-shift of 0x00ff. */
                  "    shl $8, %%ax\n"        /* AX <<= 8; */
#else
#   error "Invalid endian"
#endif
                  "    jmp 21f\n"
                  "11: movb -1(%%edi), %b0\n" /* [BYTE] Calculate offset of first miss-matched byte. */
                  "    subb -1(%%esi), %b0\n"
                  "21:\n"
                  "9:\n"
                  : "+r" (__result)
                  : "D" (__a)
                  ,"S" (__b)
                  ,"c" (__bytes)
                  : "memory");
 return (int)(__s8)__result;
}


#define __arch_memsum(p,bytes)                x86_memsum(p,bytes)
#define __arch_memcpy(dst,src,bytes)          x86_memcpy(dst,src,bytes)
#define __arch_memmove(dst,src,bytes)         x86_memmove(dst,src,bytes)
#define __arch_memcmp(a,b,bytes)              x86_memcmp(a,b,bytes)
#define __arch_memset(dst,byte,bytes)         x86_memset(dst,byte,bytes)
#define __arch_memset4(dst,dword,bytes)       x86_memset4(dst,dword,bytes)
#define __arch_memchr(haystack,needle,bytes)  x86_memchrb(haystack,(__u8)(needle),bytes)
#define __arch_memrchr(haystack,needle,bytes) x86_memrchrb(haystack,(__u8)(needle),bytes)

#define __arch_memend(haystack,needle,bytes)  x86_memendb(haystack,(__u8)(needle),bytes)
#define __arch_memlen(haystack,needle,bytes)  x86_memlenb(haystack,(__u8)(needle),bytes)
#define __arch_umemend(haystack,needle)       x86_umemendb(haystack,(__u8)(needle))
#define __arch_umemlen(haystack,needle)       x86_umemlenb(haystack,(__u8)(needle))
//todo? #define __arch_memrend(haystack,needle,bytes) x86_memrendb(haystack,(__u8)(needle),bytes)
//todo? #define __arch_memrlen(haystack,needle,bytes) x86_memrlenb(haystack,(__u8)(needle),bytes)
//todo? #define __arch_umemrend(haystack,needle)      x86_umemrendb(haystack,(__u8)(needle))
//todo? #define __arch_umemrlen(haystack,needle)      x86_umemrlenb(haystack,(__u8)(needle))

#define __arch_ffs16     x86_ffs16
#define __arch_ffs32     x86_ffs32

#define __arch_memcpy_b  x86_memcpyb
#define __arch_memcpy_w  x86_memcpyw
#define __arch_memcpy_l  x86_memcpyl
#define __arch_memset_b  x86_memsetb
#define __arch_memset_w  x86_memsetw
#define __arch_memset_l  x86_memsetl
#define __arch_memchr_b  x86_memchrb
#define __arch_memchr_w  x86_memchrw
#define __arch_memchr_l  x86_memchrl
#define __arch_memrchr_b x86_memrchrb
#define __arch_memrchr_w x86_memrchrw
#define __arch_memrchr_l x86_memrchrl
#define __arch_memcmp_b  x86_memcmpb
#define __arch_memcmp_w  x86_memcmpw
#define __arch_memcmp_l  x86_memcmpl

__DECL_END

/* Autocomplete string functions using generic constant optimizations. */
#ifndef __INTELLISENSE__
#include <kos/arch/generic/string.h>
#endif
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCHX86_STRING_H__ */
