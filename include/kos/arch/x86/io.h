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
#ifndef __KOS_ARCH_X86_IO_H__
#define __KOS_ARCH_X86_IO_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

/* Using the same trick as the linux kernel... */
#define __X86_SLOWDOWNIO_IMPL "\noutb %%al,$0x80"
#if 0
#define __X86_SLOWDOWNIO      __X86_SLOWDOWNIO_IMPL\
                              __X86_SLOWDOWNIO_IMPL\
                              __X86_SLOWDOWNIO_IMPL
#else
#define __X86_SLOWDOWNIO      __X86_SLOWDOWNIO_IMPL
#endif

#define __MAKEIN(T,sfx) \
__forcelocal T x86_in##sfx(__u16 port) {\
 T rv;\
 __asm_volatile__("in" #sfx " %w1, %0" : "=a" (rv) : "Nd" (port));\
 return rv;\
}\
__forcelocal T x86_in##sfx##_p(__u16 port) {\
 T rv;\
 __asm_volatile__("in" #sfx " %w1, %0" __X86_SLOWDOWNIO : "=a" (rv) : "Nd" (port));\
 return rv;\
}\
__forcelocal void x86_ins##sfx(__u16 port, void *addr, __size_t count) {\
 __asm_volatile__("rep; ins" #sfx \
                  : "=D" (addr)\
                  , "=c" (count)\
                  : "d"  (port)\
                  , "0"  (addr)\
                  , "1"  (count));\
}
__MAKEIN(__u8,b)
__MAKEIN(__u16,w)
__MAKEIN(__u32,l)
#undef __MAKEIN

#define __MAKEOUT(T,sfx,s1) \
__forcelocal void x86_out##sfx(__u16 port, T value) {\
 __asm_volatile__("out" #sfx " %" s1 "0, %w1"\
                  : : "a" (value), "Nd" (port));\
}\
__forcelocal void x86_out##sfx##_p(__u16 port, T value) {\
 __asm_volatile__("out" #sfx " %" s1 "0, %w1" __X86_SLOWDOWNIO\
                  : : "a" (value), "Nd" (port));\
}\
__forcelocal void x86_outs##sfx(__u16 port, void const *addr, __size_t count) {\
 __asm_volatile__("rep; outs" #sfx\
                  : "=S" (addr)\
                  , "=c" (count)\
                  : "d" (port)\
                  , "0" (addr)\
                  , "1" (count));\
}
__MAKEOUT(__u8, b,"b")
__MAKEOUT(__u16,w,"w")
__MAKEOUT(__u32,l,"")
#undef __MAKEOUT

#define karch_inb    x86_inb
#define karch_inw    x86_inw
#define karch_inl    x86_inl
#define karch_insb   x86_insb
#define karch_insw   x86_insw
#define karch_insl   x86_insl
#define karch_inb_p  x86_inb_p
#define karch_inw_p  x86_inw_p
#define karch_inl_p  x86_inl_p
#define karch_outb   x86_outb
#define karch_outw   x86_outw
#define karch_outl   x86_outl
#define karch_outsb  x86_outsb
#define karch_outsw  x86_outsw
#define karch_outsl  x86_outsl
#define karch_outb_p x86_outb_p
#define karch_outw_p x86_outw_p
#define karch_outl_p x86_outl_p

__DECL_END

#endif /* !__KOS_ARCH_X86_IO_H__ */
