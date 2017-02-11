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
#ifndef __KOS_ARCH_X86_MMU_H__
#define __KOS_ARCH_X86_MMU_H__ 1

#include <kos/compiler.h>
#include <kos/kernel/types.h>
#include <stdint.h>

__DECL_BEGIN


#ifndef __ASSEMBLY__
// Physical Pointer
typedef union __packed __x86_pptr {
 __u32 u;
 __s32 s;
} x86_pptr;

// Virtual Pointer
typedef union __packed __x86_vptr {
 __u32 u;
 __s32 s;
 struct __packed {
  unsigned int pdid:   10; /*< Page dictionary index. */
  unsigned int ptid:   10; /*< Page table index. */
  unsigned int pysoff: 12; /*< Physical offset. */
 };
} x86_vptr;
#define X86_VPTR_INIT(p) {(__u32)(__uintptr_t)(p)}

#define X86_VPTR_GET_PDID(p) ((((__uintptr_t)(p))&0xffc00000) >> 22)
#define X86_VPTR_GET_PTID(p) ((((__uintptr_t)(p))&0x003ff000) >> 12)
#define X86_VPTR_GET_POFF(p)  (((__uintptr_t)(p))&0x00000fff)
#endif /* !__ASSEMBLY__ */



// Page Directory Entry 
#define X86_PDE4DIC  1024
#define X86_PTE4PDE  1024
#ifndef __ASSEMBLY__
typedef union __packed __x86_pde {
 __s32     s;
 __u32     u;
 // Upper 20 bits describe the address of a PTE if 'present' isn't set.
 // NOTE: Not meant for use; Only meant for illustration
 union __x86_pte (*__pte)/*[X86_PTE4PDE]*/;
 struct __packed {
  unsigned int present:        1; /*< Page is present (aka. has an x86_pte associated with id). */
  unsigned int read_write:     1; /*< 1: x86_pte is read/write; 0: x86_pte is read-only. */
  unsigned int user:           1; /*< Permissions: (1: everyone may access the x86_pte; 0: only kernel can access x86_pte). */
  unsigned int write_through:  1; /*< Write-Through ability: (1: write-through enabled; 0: write-back enabled). */
  unsigned int cache_disabled: 1; /*< Set to disable caching of this x86_pte. */
  unsigned int accessed:       1; /*< (Set by the CPU): When set, the x86_pte has been read-from/written-to. (Must be reset by the kernel). */
  unsigned int zero:           1; /*< Set to 0. */
  unsigned int size:           1; /*< Page size (0: 4 kilobyte; 1: 4 megabyte (requires PSE)). */
  unsigned int global:         1; /*< Ignored. */
  unsigned int avail:          3; /*< Unused by the CPU, may be used by the kernel. */
  unsigned int pte:           20; /*< 4-kilobyte aligned Page table entry address (cast to 'pt (*)[X86_PTE4PDE]'). */
 };
 struct __packed {
  unsigned int flags:     12;
  unsigned int __unnamed: 20;
 };
} x86_pde;
#define x86_pde_getpte(self) ((x86_pte *)(uintptr_t)((self)->u&X86_PDE_MASK_PTE))
#endif /* !__ASSEMBLY__ */

// Flags matching the offsets in the structure above
#define X86_PDE_MASK_PTE            UINT32_C(0xfffff000)
#define X86_PDE_MASK_FLAGS          UINT32_C(0x00000fff)
#define X86_PDE_FLAG_GLOBAL         UINT32_C(0x00000100)
#define X86_PDE_FLAG_SIZE           UINT32_C(0x00000080)
#define X86_PDE_FLAG_ACCESSED       UINT32_C(0x00000020)
#define X86_PDE_FLAG_CACHE_DISABLED UINT32_C(0x00000010)
#define X86_PDE_FLAG_WRITE_THROUGH  UINT32_C(0x00000008)
#define X86_PDE_FLAG_USER           UINT32_C(0x00000004)
#define X86_PDE_FLAG_READ_WRITE     UINT32_C(0x00000002)
#define X86_PDE_FLAG_PRESENT        UINT32_C(0x00000001)


// Page Table Entry (Must be 4-kilobyte aligned)
#ifndef __ASSEMBLY__
typedef union __packed __x86_pte {
 __s32      s;
 __u32      u;
 // Upper 20 bits describe a physical address.
 // NOTE: Not meant for use; Only meant for illustration
 union __x86_pptr *__pptr;
 struct __packed {
  unsigned int present:        1; /*< Page is present. */
  unsigned int read_write:     1; /*< 1: page is read/write; 0: page is read-only. */
  unsigned int user:           1; /*< Permissions: (1: everyone may access the page; 0: only kernel can access page). */
  unsigned int write_through:  1; /*< Write-Through ability: (1: write-through enabled; 0: write-back enabled). */
  unsigned int cache_disabled: 1; /*< Set to disable caching of this page. */
  unsigned int accessed:       1; /*< (Set by the CPU): When set, the page has been read-from/written-to. (Must be reset by the kernel). */
  unsigned int dirty:          1; /*< (Set by the CPU): When set, the page has been written-to. (Must be reset by the kernel). */
  unsigned int zero:           1; /*< Set to 0. */
  unsigned int global:         1; /*< Ignored. */
  unsigned int avail:          3; /*< Unused by the CPU, may be used by the kernel. */
  unsigned int pptr:          20; /*< 4-kilobyte aligned physical memory address. */
 };
 struct __packed {
  unsigned int flags:     12;
  unsigned int __unnamed: 20;
 };
} x86_pte;
#define x86_pte_getpptr(self) ((uintptr_t)((self)->u&X86_PTE_MASK_PPTR))
#endif /* !__ASSEMBLY__ */

// Flags matching the offsets in the structure above
#define X86_PTE_MASK_PPTR           UINT32_C(0xfffff000)
#define X86_PTE_MASK_FLAGS          UINT32_C(0x00000fff)
#define X86_PTE_FLAG_GLOBAL         UINT32_C(0x00000100)
#define X86_PTE_FLAG_DIRTY          UINT32_C(0x00000040)
#define X86_PTE_FLAG_ACCESSED       UINT32_C(0x00000020)
#define X86_PTE_FLAG_CACHE_DISABLED UINT32_C(0x00000010)
#define X86_PTE_FLAG_WRITE_THROUGH  UINT32_C(0x00000008)
#define X86_PTE_FLAG_USER           UINT32_C(0x00000004)
#define X86_PTE_FLAG_READ_WRITE     UINT32_C(0x00000002)
#define X86_PTE_FLAG_PRESENT        UINT32_C(0x00000001)

#ifndef __ASSEMBLY__
#define __arch_getpagedirectory()  __xblock({ void *_r; __asm__("movl %%cr3, %0" : "=r" (_r)); __xreturn _r; })
#define __arch_setpagedirectory(p) __xblock({           __asm__("movl %0, %%cr3" : : "r" (p));               })

#define __arch_pagingenabled() __xblock({ __u32 _cr0; __asm__("movl %%cr0, %0" : "=r" (_cr0)); __xreturn (_cr0&0x80000000)!=0; })
#define __arch_enablepaging()  __xblock({ __u32 _cr0; __asm__("movl %%cr0, %0" : "=r" (_cr0)); _cr0 |= (0x80000000); __asm__("movl %0, %%cr0" : : "r" (_cr0)); (void)0; })
#define __arch_disablepaging() __xblock({ __u32 _cr0; __asm__("movl %%cr0, %0" : "=r" (_cr0)); _cr0 &= (0x7FFFFFFF); __asm__("movl %0, %%cr0" : : "r" (_cr0)); (void)0; })
#else /* !__ASSEMBLY__ */
#define __arch_enablepaging()  __arch_enablepaging
#define __arch_disablepaging() __arch_disablepaging
.macro __arch_enablepaging
    push %eax
    mov  %cr0, %eax
    or   %eax, $0x80000000
    mov  %eax, %cr0
    pop  %eax
.endm
.macro __arch_disablepaging
    push %eax
    mov  %cr0, %eax
    and  %eax, $0x7FFFFFFF
    mov  %eax, %cr0
    pop  %eax
.endm

// Translate a virtual address into its physical counterpart
// NOTES:
//  - None of the given registers may overlap
//  - Does not make use of the stack
//  - Paging must be disabled (cr0&0x80000000 == 0), but the page
//    directory used for translation must still be stored in CR3.
// >> This macro is used to implement a safe way of switching to
//    physical addresses after encountering an interrupt in usercode.
.macro TRANSLATE_VADDR rIN, rOUT, rCLOB
    movI %cr3, \rOUT
    movI \rIN, \rCLOB
    // X86_VPTR_GET_PDID: ((\rCLOB & 0xffc00000) >> 22) * sizeof(x86_pte)
    andI $0xffc00000, \rCLOB
#if __SIZEOF_POINTER__ == 4
    shrI $20, \rCLOB
#elif __SIZEOF_POINTER__ == 8
    shrI $19, \rCLOB
#else
#   error __SIZEOF_POINTER__
#endif
    addI \rCLOB, \rOUT
    movI (\rOUT), \rOUT
    andI $X86_PDE_MASK_PTE, \rOUT
    // We now have the address of the PTE stored in \rOUT
    // >> Now add the PTID offset
    movI \rIN, \rCLOB
    // X86_VPTR_GET_PTID: ((\rCLOB & 0x003ff000) >> 12) * sizeof(x86_pde)
    andI $0x003ff000, \rCLOB
#if __SIZEOF_POINTER__ == 4
    shrI $10, \rCLOB
#elif __SIZEOF_POINTER__ == 8
    shrI $9, \rCLOB
#else
#   error __SIZEOF_POINTER__
#endif
    addI \rCLOB, \rOUT
    movI (\rOUT), \rOUT
    andI $X86_PTE_MASK_PPTR, \rOUT
    // Add the pptr offset from EAX
    movI \rIN, \rCLOB
    andI $0x00000fff, \rCLOB
    addI \rCLOB, \rOUT
.endm
#endif /* !__ASSEMBLY__ */

__DECL_END

#endif /* !__KOS_ARCH_X86_MMU_H__ */
