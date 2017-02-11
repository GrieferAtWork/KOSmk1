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
#ifndef __KOS_ASM_H__
#define __KOS_ASM_H__ 1

#include <kos/compiler.h>
#include <kos/config.h>

__DECL_BEGIN

#ifdef __ASSEMBLY__
#define __ASM_TEXT(x) x;
#else
#define __ASM_TEXT(x) "\t" #x "\n"
#endif


#define __O8(x)  x##b
#define __O16(x) x##w
#define __O32(x) x##l
#define __O64(x) x##q

#if __SIZEOF_POINTER__ == 1
#   define __I __O8
#elif __SIZEOF_POINTER__ == 2
#   define __I __O16
#elif __SIZEOF_POINTER__ == 4
#   define __I __O32
#elif __SIZEOF_POINTER__ == 8
#   define __I __O64
#endif

/*[[[deemon
local ops = list {
   "mov","push","pop","lea",
   "add","sub","mul","div",
   "pushf","popf","and","or","xor",
   "shl","shr","xchg",
};
local longname = ((for (local x: ops) #x) > ...)+2;
for (local o: ops) {
  print "#define "+(o+"__I ").ljust(longname)+"      __I("+o+")";
  print "#define "+(o+"8 ").ljust(longname)+"     __O8("+o+")";
  print "#define "+(o+"16").ljust(longname)+"    __O16("+o+")";
  print "#define "+(o+"32").ljust(longname)+"    __O32("+o+")";
  print "#define "+(o+"64").ljust(longname)+"    __O64("+o+")";
}
]]]*/
#define movI         __I(mov)
#define mov8        __O8(mov)
#define mov16      __O16(mov)
#define mov32      __O32(mov)
#define mov64      __O64(mov)
#define pushI        __I(push)
#define push8       __O8(push)
#define push16     __O16(push)
#define push32     __O32(push)
#define push64     __O64(push)
#define popI         __I(pop)
#define pop8        __O8(pop)
#define pop16      __O16(pop)
#define pop32      __O32(pop)
#define pop64      __O64(pop)
#define leaI         __I(lea)
#define lea8        __O8(lea)
#define lea16      __O16(lea)
#define lea32      __O32(lea)
#define lea64      __O64(lea)
#define addI         __I(add)
#define add8        __O8(add)
#define add16      __O16(add)
#define add32      __O32(add)
#define add64      __O64(add)
#define subI         __I(sub)
#define sub8        __O8(sub)
#define sub16      __O16(sub)
#define sub32      __O32(sub)
#define sub64      __O64(sub)
#define mulI         __I(mul)
#define mul8        __O8(mul)
#define mul16      __O16(mul)
#define mul32      __O32(mul)
#define mul64      __O64(mul)
#define divI         __I(div)
#define div8        __O8(div)
#define div16      __O16(div)
#define div32      __O32(div)
#define div64      __O64(div)
#define pushfI       __I(pushf)
#define pushf8      __O8(pushf)
#define pushf16    __O16(pushf)
#define pushf32    __O32(pushf)
#define pushf64    __O64(pushf)
#define popfI        __I(popf)
#define popf8       __O8(popf)
#define popf16     __O16(popf)
#define popf32     __O32(popf)
#define popf64     __O64(popf)
#define andI         __I(and)
#define and8        __O8(and)
#define and16      __O16(and)
#define and32      __O32(and)
#define and64      __O64(and)
#define orI          __I(or)
#define or8         __O8(or)
#define or16       __O16(or)
#define or32       __O32(or)
#define or64       __O64(or)
#define xorI         __I(xor)
#define xor8        __O8(xor)
#define xor16      __O16(xor)
#define xor32      __O32(xor)
#define xor64      __O64(xor)
#define shlI         __I(shl)
#define shl8        __O8(shl)
#define shl16      __O16(shl)
#define shl32      __O32(shl)
#define shl64      __O64(shl)
#define shrI         __I(shr)
#define shr8        __O8(shr)
#define shr16      __O16(shr)
#define shr32      __O32(shr)
#define shr64      __O64(shr)
#define xchgI        __I(xchg)
#define xchg8       __O8(xchg)
#define xchg16     __O16(xchg)
#define xchg32     __O32(xchg)
#define xchg64     __O64(xchg)
//[[[end]]]

#define LOCAL_FUNCTION(name)                .type name, @function; name
#define GLOBAL_FUNCTION(name) .global name; .type name, @function; name
#define GLOBAL_SYMBOL(name)   .global name; name
#define SECTION(name)         .section name
#define SYMBOLEND(name)       .size name, . - name

#define POPARGS(n)            addI $((n)*__SIZEOF_POINTER__), %esp

#ifdef __DEBUG__
#   define DEBUGNOP         nop
#   define STACKOFF         __SIZEOF_POINTER__
#   define STACKARG(i)   (((i)+2)*__SIZEOF_POINTER__)(%ebp)
#   define STACKADDR(reg,i) movI %ebp, reg; addl $(((i)+2)*__SIZEOF_POINTER__), reg
#   define STACKBEGIN       pushI %ebp; movI %esp, %ebp;
#   define STACKEND         leave
//#   define STACKEND       movI %ebp, %esp; popI %ebp;
#else
#   define DEBUGNOP         /* nothing */
#   define STACKOFF         0
#   define STACKARG(i)   (((i)+1)*__SIZEOF_POINTER__)(%esp)
#   define STACKADDR(reg,i) movI %ebp, reg; addl $(((i)+1)*__SIZEOF_POINTER__), reg
#   define STACKBEGIN       /* nothing */
#   define STACKEND         /* nothing */
#endif

#ifdef __ASSEMBLY__
#   define I    __I
#   define O8   __O8
#   define O16  __O16
#   define O32  __O32
#   define O64  __O64
#   define PS   __SIZEOF_POINTER__
#endif

__DECL_END

#endif /* !__KOS_ASM_H__ */
