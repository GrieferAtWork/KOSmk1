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
#ifndef __KOS_KERNEL_DEBUG_C__
#define __KOS_KERNEL_DEBUG_C__ 1

#include <kos/kernel/debug.h>

#ifdef __DEBUG__
#include <assert.h>
#include <ctype.h>
#include <itos.h>
#include <kos/compiler.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/task.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/tty.h>
#include <kos/kernel/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

__DECL_BEGIN

#ifdef __KERNEL_HAVE_DEBUG_STACKCHECKS
extern __u8 stack_top[];
extern __u8 stack_bottom[];

static __noinline void
__debug_checkalloca_x_impl(__size_t bytes __LIBC_DEBUG_X__PARAMS);

#ifndef __LIBC_DEBUG_X__ARGS
__noinline void
debug_checkalloca_d(__size_t bytes __LIBC_DEBUG__PARAMS) {
#ifdef __DEBUG__
 struct __libc_debug dbg = {__LIBC_DEBUG_FILE,__LIBC_DEBUG_LINE,__LIBC_DEBUG_FUNC};
 __debug_checkalloca_x_impl(bytes,&dbg);
#else
 __debug_checkalloca_x_impl(bytes __LIBC_DEBUG__NULL);
#endif
}
#endif /* !__LIBC_DEBUG_X__ARGS */

#ifdef __LIBC_DEBUG_X__ARGS
__noinline void
debug_checkalloca_x(__size_t bytes __LIBC_DEBUG_X__PARAMS) {
 __debug_checkalloca_x_impl(bytes __LIBC_DEBUG_X__FWD);
}
#endif /* __LIBC_DEBUG_X__ARGS */

static __noinline void
__debug_checkalloca_x_impl(__size_t bytes __LIBC_DEBUG_X__PARAMS) {
 struct ktask *task_self = ktask_self();
 uintptr_t base_addr,alloc_addr;
 __asm_volatile__("movl %%ebp, %0" : "=r" (base_addr));
 alloc_addr = base_addr-bytes;
 __assert_xatf("alloca(...)",2,alloc_addr <= base_addr
              ,"Stack overflow while alloca-ing %Iu (%#Ix) bytes\n"
               ">> Negative address offset by %Iu (%#Ix) bytes"
              ,bytes,bytes,alloc_addr-base_addr,alloc_addr-base_addr);
 if (task_self) {
  kassert_ktask(task_self);
#if 1
  /* Must ignore out-of-bounds for callbacks from places
   * where a different stack is used, than is specified.
   * e.g.: -> 'ktask_unschedule_aftercrit'
   *       -> 'ktask_ononterminate'
   *          // At this point, the real calling task is no longer a
   *          // valid task as registered in kcpu_self()->c_current,
   *          // but instead a dangling task only held alive by
   *          // the fact that interrupts are disabled
   *       -> 'kproc_deltask'
   *       -> 'kproc_close'
   *       -> 'kprocmodules_quit'
   *       -> 'kshlib_decref'
   *       -> 'kshlib_destroy'
   *       -> 'ksymtable_quit'
   *       -> '_free_d'
   *       -> 'mall_free'
   *       -> 'k_dosyslogf'
   *       -> 'k_dovsyslogf'
   *       -> 'format_vprintf'
   *       -> 'syslog_callback'
   *       -> 'k_dosyslog'
   *       -> 'alloca' (For line prefix buffer)
   *       -> 'debug_checkalloca_d' (And here we are!)
   */
  if (base_addr >= (uintptr_t)task_self->t_kstackend ||
      base_addr < (uintptr_t)task_self->t_kstack) return;
#else
  __assert_atf("alloca(...)",1
              ,(uintptr_t)task_self->t_kstackend > base_addr
              ,"Stack overflow while alloca-ing %Iu (%#Ix) bytes\n"
               ">> Old stack is already out-of-bounds above by %Iu (%#Ix) bytes"
              ,bytes,bytes
              ,base_addr-(uintptr_t)task_self->t_kstackend
              ,base_addr-(uintptr_t)task_self->t_kstackend);
  __assert_atf("alloca(...)",1
              ,(uintptr_t)task_self->t_kstack <= base_addr
              ,"Stack overflow while alloca-ing %Iu (%#Ix) bytes\n"
               ">> Old stack is already out-of-bounds below by %Iu (%#Ix) bytes"
              ,bytes,bytes
              ,(uintptr_t)task_self->t_kstack-base_addr
              ,(uintptr_t)task_self->t_kstack-base_addr);
#endif
  __assert_xatf("alloca(...)",2
               ,(uintptr_t)task_self->t_kstack <= alloc_addr
               ,"Stack overflow while alloca-ing %Iu (%#Ix) bytes\n"
                ">> cannot accomodate %Iu (%#Ix) bytes"
               ,bytes,bytes
               ,(uintptr_t)task_self->t_kstack-alloc_addr
               ,(uintptr_t)task_self->t_kstack-alloc_addr);
 } else if (base_addr >= (uintptr_t)stack_bottom &&
            base_addr < (uintptr_t)stack_top) {
  __assert_xatf("alloca(...)",2
               ,(uintptr_t)stack_bottom <= alloc_addr
               ,"Stack overflow while alloca-ing %Iu (%#Ix) bytes\n"
                ">> cannot accomodate %Iu (%#Ix) bytes"
               ,bytes,bytes
               ,(uintptr_t)stack_bottom-alloc_addr
               ,(uintptr_t)stack_bottom-alloc_addr);
 }
}
#endif /* __KERNEL_HAVE_DEBUG_STACKCHECKS */

#define OUT(s,max) (serial_printn(SERIAL_01,s,max)/*,tty_printn(s,max)*/)
void debug_hexdump(void const *p, size_t s) {
 byte_t line[16],temp;
 size_t i,line_size,spaceend;
 char ascii_mem[17],hexch[3];
 kassertmem(p,s);
 hexch[2] = ' ';
 while (1) {
  line_size = s < sizeof(line) ? s : sizeof(line);
  memcpy(line,p,line_size);
  for (i = 0; i < line_size; ++i) {
   temp = (line[i] & 0xF0) >> 4; hexch[0] = temp >= 10 ? 'A'+(temp-10) : '0'+temp;
   temp =  line[i] & 0x0F;       hexch[1] = temp >= 10 ? 'A'+(temp-10) : '0'+temp;
   OUT(hexch,3);
  }
  // Fill unused space
  spaceend = 3*(sizeof(line)-line_size);
  for (i = 0; i < spaceend; ++i) OUT(" ",1);
  memset(ascii_mem,' ',sizeof(ascii_mem)-sizeof(char));
  for (i = 0; i < line_size; ++i) ascii_mem[i] = isgraph(line[i]) ? line[i] : '.';
  ascii_mem[line_size] = '\n';
  OUT(ascii_mem,line_size+1);
  if (s <= sizeof(line)) break;
  s -= sizeof(line);
  *((char const **)&p) += sizeof(line);
 }
}
#undef OUT

//#define KMEM_DEBUG_CALL_FROM_NON_RING_ZERO


kerrno_t
kmem_validatebyte(__kernel byte_t const *__restrict p) {
 if __unlikely(!p) return KS_EMPTY;
#ifdef KMEM_DEBUG_CALL_FROM_NON_RING_ZERO
 if __unlikely(ksegment_currentring() != 0) return KE_OK;
#endif
 if __unlikely(kpagedir_current() != kpagedir_kernel()) return KE_OK;
 if __unlikely(!kpagedir_ismapped(kpagedir_kernel(),
              (void *)alignd((uintptr_t)p,PAGEALIGN),1)) return KE_RANGE;
 if __unlikely(kpageframe_isfreepage(p,1)) return KE_INVAL;
 return KE_OK;
}
kerrno_t
kmem_validate(__kernel void const *__restrict p, size_t s) {
 uintptr_t aligned_p;
 if __unlikely(!s) return KE_OK;
 if __unlikely(!p) return KS_EMPTY;
#ifdef KMEM_DEBUG_CALL_FROM_NON_RING_ZERO
 if __unlikely(ksegment_currentring() != 0) return KE_OK;
#endif
 if __unlikely(kpagedir_current() != kpagedir_kernel()) return KE_OK;
 aligned_p = alignd((uintptr_t)p,PAGEALIGN);
 if __unlikely(!kpagedir_ismapped(kpagedir_kernel(),(void *)aligned_p,
               ceildiv(((uintptr_t)p-aligned_p)+s,PAGESIZE))) return KE_RANGE;
 if __unlikely(kpageframe_isfreepage(p,s)) return KE_INVAL;
 return KE_OK;
}




// Helper functions designed to be called from assembly
COMPILER_PACK_PUSH(1)
struct __packed irettail {
 __uintptr_t eip,cs,eflags,useresp,ss;
};
COMPILER_PACK_POP

void printirettail(struct irettail *tail) {
 printf("printirettail(%#p) {\n",tail);
 printf("\teip     = %#Ix\n",tail->eip);
 printf("\tcs      = %#Ix\n",tail->cs);
 printf("\teflags  = %#Ix\n",tail->eflags);
 printf("\tuseresp = %#Ix\n",tail->useresp);
 printf("\tss      = %#Ix\n",tail->ss);
 printf("\tcr3     = %#Ix\n",kpagedir_current());
 printf("}\n");
}
void printregs(struct ktaskregisters3 *regs) {
 printf("printregs(%#p) {\n",regs);
 printf("\tbase.ds     = %#Ix\n",regs->base.ds);
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
 printf("\tbase.es     = %#Ix\n",regs->base.es);
 printf("\tbase.fs     = %#Ix\n",regs->base.fs);
 printf("\tbase.gs     = %#Ix\n",regs->base.gs);
#endif
 printf("\tbase.edi    = %#Ix\n",regs->base.edi);
 printf("\tbase.esi    = %#Ix\n",regs->base.esi);
 printf("\tbase.ebp    = %#Ix\n",regs->base.ebp);
 printf("\tbase.ebx    = %#Ix\n",regs->base.ebx);
 printf("\tbase.edx    = %#Ix\n",regs->base.edx);
 printf("\tbase.ecx    = %#Ix\n",regs->base.ecx);
 printf("\tbase.eax    = %#Ix\n",regs->base.eax);
 printf("\tbase.eip    = %#Ix\n",regs->base.eip);
 printf("\tbase.cs     = %#Ix\n",regs->base.cs);
 printf("\tbase.eflags = %#Ix\n",regs->base.eflags);
 printf("\tuseresp     = %#Ix\n",regs->useresp);
 printf("\tss          = %#Ix\n",regs->ss);
 printf("}\n");
}

void printaddr(void *p) { printf("addr = %p\n",p); }
void HERE(void) { printf("HERE\n"); tb_printex(1); }

__DECL_END
#endif /* __DEBUG__ */

#endif /* !__KOS_KERNEL_DEBUG_C__ */
