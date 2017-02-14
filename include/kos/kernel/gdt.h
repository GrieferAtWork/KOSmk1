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
#ifndef __KOS_KERNEL_GDT_H__
#define __KOS_KERNEL_GDT_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>
#include <assert.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
__COMPILER_PACK_PUSH(1)

// Segment Descriptor / GDT (Global Descriptor Table) Entry
struct __packed ksegment {
union __packed {
struct __packed { __u32 ul32,uh32; };
struct __packed { __s32 sl32,sh32; };
 __s64 s;
 __u64 u;
struct __packed {
 __u16        sizelow;
 unsigned int baselow:    24;
union __packed {
 __u8         access;
struct __packed {
 unsigned int type:       4;
 unsigned int __unnamed2: 4;
};struct __packed {
 // Just set to 0. The CPU sets this to 1 when the segment is accessed.
 unsigned int accessed:    1; /*< Accessed bit. */
 // Readable bit for code selectors:
 //   Whether read access for this segment is allowed.
 //   Write access is never allowed for code segments.
 // Writable bit for data selectors:
 //   Whether write access for this segment is allowed.
 //   Read access is always allowed for data segments. 
 unsigned int rw:          1; /*< Also the busy bit of tasks. */
 // Direction bit for data selectors:
 //   Tells the direction. 0 the segment grows up. 1 the segment
 //   grows down, ie. the offset has to be greater than the limit.
 // Conforming bit for code selectors:
 //     If 1: code in this segment can be executed from an equal or lower
 //           privilege level. For example, code in ring 3 can far-jump to
 //           conforming code in a ring 2 segment. The privl-bits represent
 //           the highest privilege level that is allowed to execute the segment.
 //           For example, code in ring 0 cannot far-jump to a conforming code
 //           segment with privl==0x2, while code in ring 2 and 3 can. Note that
 //           the privilege level remains the same, ie. a far-jump form ring
 //           3 to a privl==2-segment remains in ring 3 after the jump.
 //     If 0: code in this segment can only be executed from the ring set in privl. 
 unsigned int dc:          1; /*< Direction bit/Conforming bit. */
 unsigned int execute:     1; /*< Executable bit. If 1 code in this segment can be executed, ie. a code selector. If 0 it is a data selector. */
 unsigned int system:      1; /*< 0: System descriptor; 1: Code/Data/Stack (always set to 1) */
 unsigned int privl:       2; /*< Privilege, 2 bits. Contains the ring level, 0 = highest (kernel), 3 = lowest (user applications). */
 unsigned int present:     1; /*< Present bit. This must be 1 for all valid selectors. */
};};
union __packed {
struct __packed {
 unsigned int __unnamed1:  4;
 unsigned int flags:       4;
};struct __packed {
 unsigned int sizehigh:    4;
 unsigned int available:   1; /*< Availability bit (Set to 1). */
 unsigned int longmode:    1; /*< Set to 0 (Used in x86-64). */
 // Indicates how the instructions(80386 and up) access register and memory data in protected mode.
 // If 0: instructions are 16-bit instructions, with 16-bit offsets and 16-bit registers.
 //       Stacks are assumed 16-bit wide and SP is used.
 // If 1: 32-bits are assumed.
 // Allows 8086-80286 programs to run.
 unsigned int dbbit:       1;
 // If 0: segments can be 1 byte to 1MB in length.
 // If 1: segments can be 4KB to 4GB in length. 
 unsigned int granularity: 1;
};};
 __u8         basehigh;
};};};

struct __packed kidtpointer {
 __u16            limit;
 struct ksegment *base;
};

__COMPILER_PACK_POP
#endif /* !__ASSEMBLY__ */

// Access flags
#define SEG_ACCESS_PRESENT     0x00008000 /*< present. */
#define SEG_ACCESS_PRIVL(n) (((n)&0x3) << 13) /*< privl (mask: 0x00006000). */
#define SEG_ACCESS_SYSTEM      0x00001000 /*< system. */
#define SEG_ACCESS_EXECUTE     0x00000800 /*< execute. */
#define SEG_ACCESS_DC          0x00000400 /*< dc. */
#define SEG_ACCESS_RW          0x00000200 /*< rw. */
#define SEG_ACCESS_ACCESSED    0x00000100 /*< accessed. */

#define SEG_ACCESS(ex,dc,rw,ac) \
 (((ex)?SEG_ACCESS_EXECUTE:0)|((dc)?SEG_ACCESS_DC:0)|\
  ((rw)?SEG_ACCESS_RW:0)|((ac)?SEG_ACCESS_ACCESSED:0))

#define SEG_DATA_RD        SEG_ACCESS(0,0,0,0) // Read-Only
#define SEG_DATA_RDA       SEG_ACCESS(0,0,0,1) // Read-Only, accessed
#define SEG_DATA_RDWR      SEG_ACCESS(0,0,1,0) // Read/Write
#define SEG_DATA_RDWRA     SEG_ACCESS(0,0,1,1) // Read/Write, accessed
#define SEG_DATA_RDEXPD    SEG_ACCESS(0,1,0,0) // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   SEG_ACCESS(0,1,0,1) // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  SEG_ACCESS(0,1,1,0) // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA SEG_ACCESS(0,1,1,1) // Read/Write, expand-down, accessed
#define SEG_CODE_EX        SEG_ACCESS(1,0,0,0) // Execute-Only
#define SEG_CODE_EXA       SEG_ACCESS(1,0,0,1) // Execute-Only, accessed
#define SEG_CODE_EXRD      SEG_ACCESS(1,0,1,0) // Execute/Read
#define SEG_CODE_EXRDA     SEG_ACCESS(1,0,1,1) // Execute/Read, accessed
#define SEG_CODE_EXC       SEG_ACCESS(1,1,0,0) // Execute-Only, conforming
#define SEG_CODE_EXCA      SEG_ACCESS(1,1,0,1) // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     SEG_ACCESS(1,1,1,0) // Execute/Read, conforming
#define SEG_CODE_EXRDCA    SEG_ACCESS(1,1,1,1) // Execute/Read, conforming, accessed

// Flags
#define SEG_FLAG_GRAN      0x00800000 /*< Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB). */
#define SEG_FLAG_32BIT     0x00400000 /*< dbbit = 1. */
#define SEG_FLAG_LONGMODE  0x00200000 /*< longmode = 1. */
#define SEG_FLAG_AVAILABLE 0x00100000 /*< available = 1. */

//////////////////////////////////////////////////////////////////////////
// Useful predefined segment configurations
// NOTE: The following configs match what is described here: http://wiki.osdev.org/Getting_to_Ring_3
//    >> THIS IS CONFIRMED!
#define SEG_CODE_PL0    (SEG_FLAG_32BIT|SEG_FLAG_AVAILABLE|SEG_FLAG_GRAN|SEG_ACCESS_SYSTEM|SEG_ACCESS_PRESENT|SEG_ACCESS_PRIVL(0)|SEG_CODE_EXRD)
#define SEG_DATA_PL0    (SEG_FLAG_32BIT|SEG_FLAG_AVAILABLE|SEG_FLAG_GRAN|SEG_ACCESS_SYSTEM|SEG_ACCESS_PRESENT|SEG_ACCESS_PRIVL(0)|SEG_DATA_RDWR)
#define SEG_CODE_PL3    (SEG_FLAG_32BIT|SEG_FLAG_AVAILABLE|SEG_FLAG_GRAN|SEG_ACCESS_SYSTEM|SEG_ACCESS_PRESENT|SEG_ACCESS_PRIVL(3)|SEG_CODE_EXRD)
#define SEG_DATA_PL3    (SEG_FLAG_32BIT|SEG_FLAG_AVAILABLE|SEG_FLAG_GRAN|SEG_ACCESS_SYSTEM|SEG_ACCESS_PRESENT|SEG_ACCESS_PRIVL(3)|SEG_DATA_RDWR)
#define SEG_CODE_PL0_16 (                             SEG_FLAG_AVAILABLE|SEG_ACCESS_SYSTEM|SEG_ACCESS_PRESENT|SEG_ACCESS_PRIVL(0)|SEG_CODE_EXRD)
#define SEG_DATA_PL0_16 (                             SEG_FLAG_AVAILABLE|SEG_ACCESS_SYSTEM|SEG_ACCESS_PRESENT|SEG_ACCESS_PRIVL(0)|SEG_DATA_RDWR)
#define SEG_TSS         (                                                                  SEG_ACCESS_PRESENT|SEG_ACCESS_PRIVL(3)|SEG_CODE_EXA)

#define SEG_LIMIT_MAX  0x000fffff

#ifndef __ASSEMBLY__
#define __SEG_ENCODELO(base,size,config) \
 (((__u32)((size)&0xffff))|      /* 0x0000ffff */\
  ((__u32)((base)&0xffff) << 16) /* 0xffff0000 */)
#define __SEG_ENCODEHI(base,size,config) \
 ((((__u32)(base)&0x00ff0000) >> 16)| /* 0x000000ff */\
  ((__u32)(config)&0x00f0ff00)|       /* 0x00f0ff00 */\
  ((__u32)(size)&0x000f0000)|         /* 0x000f0000 */\
  ((__u32)(base)&0xff000000)          /* 0xff000000 */)


#define KSEGMENT_INIT(base,size,config) \
 {{{ __SEG_ENCODELO(base,size,config),  \
     __SEG_ENCODEHI(base,size,config) }}}

__local __nonnull((1)) void
ksegment_encode(struct ksegment *self, __u32 base,
                __u32 size, __u32 config);


#ifndef __INTELLISENSE__
__local __nonnull((1)) void
ksegment_encode(struct ksegment *self, __u32 base,
                __u32 size, __u32 config) {
 assertf(size <= SEG_LIMIT_MAX,"Size %I32u is too large",size);
 self->ul32 = __SEG_ENCODELO(base,size,config);
 self->uh32 = __SEG_ENCODEHI(base,size,config);
}
#endif

#ifdef __MAIN_C__
// Initialize the GDT
extern __crit void kernel_initialize_gdt(void);
#endif
#endif /* !__ASSEMBLY__ */

#ifndef __INTELLISENSE__
__STATIC_ASSERT(sizeof(struct ksegment) == 8);
#endif

#define KSEGMENT_KERNEL_CODE    (1*8) // 0x08
#define KSEGMENT_KERNEL_DATA    (2*8) // 0x10
#define KSEGMENT_USER_CODE      (3*8) // 0x18
#define KSEGMENT_USER_DATA      (4*8) // 0x20
#define KSEGMENT_KERNEL_CODE_16 (5*8) // 0x28
#define KSEGMENT_KERNEL_DATA_16 (6*8) // 0x30
#define KSEGMENT_KERNELTSS      (7*8) // 0x38

__DECL_END
#endif /* __KERNEL__ */

#define ksegment_currentring() \
 __xblock({ __u16 __cs;\
            __asm_volatile__("movw %%cs, %0" : "=r" (__cs));\
            __xreturn __cs&0x3;\
 })

#ifdef __ASSEMBLY__
.macro SET_KERNEL_SEGMENTS
    mov $KSEGMENT_KERNEL_DATA, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
.endm
#endif


#endif /* !__KOS_KERNEL_GDT_H__ */
