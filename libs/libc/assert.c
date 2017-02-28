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
#ifndef __ASSERT_C__
#define __ASSERT_C__ 1

#include <kos/config.h>
#ifdef __DEBUG__
#include <assert.h>
#include <kos/assert.h>
#include <kos/compiler.h>
#include <kos/syslog.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <traceback.h>

#ifdef __KERNEL__
#include <kos/arch/asm.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/task.h>
#include <kos/kernel/tty.h>
#ifdef __x86__
#include <kos/arch/x86/vga.h>
#endif
#else
#include <unistd.h>
#include <stdlib.h>
#endif

#ifndef KDEBUG_SOURCEPATH_PREFIX
#define KDEBUG_SOURCEPATH_PREFIX "../"
#endif

__DECL_BEGIN

#ifndef USE_USERSPACE_SYSLOG
#define USE_USERSPACE_SYSLOG 1
#endif


#if defined(__KERNEL__) && defined(KLOG_RAW)
#   define ASSERT_PRINTF(...)           (k_syslogf(KLOG_RAW,__VA_ARGS__),tty_printf(__VA_ARGS__))
#   define ASSERT_VPRINTF(format,args) (k_vsyslogf(KLOG_RAW,format,args),tty_vprintf(format,args))
#elif defined(__KERNEL__)
#   define ASSERT_PRINTF(...)           (serial_printf(SERIAL_01,__VA_ARGS__),tty_printf(__VA_ARGS__))
#   define ASSERT_VPRINTF(format,args) (serial_vprintf(SERIAL_01,format,args),tty_vprintf(format,args))
#elif USE_USERSPACE_SYSLOG
#   define ASSERT_PRINTF(...)           (dprintf(STDERR_FILENO,__VA_ARGS__),k_syslogf(KLOG_ERROR,__VA_ARGS__))
#   define ASSERT_VPRINTF(format,args) (vdprintf(STDERR_FILENO,format,args),k_vsyslogf(KLOG_ERROR,format,args))
#else
#   define ASSERT_PRINTF(...)           dprintf(STDERR_FILENO,__VA_ARGS__)
#   define ASSERT_VPRINTF(format,args) vdprintf(STDERR_FILENO,format,args)
#endif


__public void __builtin_unreachable_d(__LIBC_DEBUG_PARAMS) {
 ASSERT_PRINTF(KDEBUG_SOURCEPATH_PREFIX
               "%s(%d) : %s : Unreachable code reached\n"
               ,__LIBC_DEBUG_FILE
               ,__LIBC_DEBUG_LINE
               ,__LIBC_DEBUG_FUNC);
 tb_printex(1);
#ifdef __KERNEL__
 arch_hang();
#else
 abort();
#endif
}

__public __noinline __noreturn
void __assertion_failedf(__LIBC_DEBUG_PARAMS_ char const *expr,
                         unsigned int skip, char const *fmt, ...) {
#ifdef __KERNEL__
 karch_irq_disable();
#ifdef __x86__ // (Light) blue screen!
 tty_x86_setcolors(vga_entry_color(VGA_COLOR_WHITE,VGA_COLOR_LIGHT_BLUE));
#endif
 tty_clear();
 //serial_print(SERIAL_01,"\n");
#endif
 ASSERT_PRINTF("===================================================\n"
               "    Assertion failed:\n");
 if (__LIBC_DEBUG_FILE || __LIBC_DEBUG_FUNC) {
  ASSERT_PRINTF("    " KDEBUG_SOURCEPATH_PREFIX "%s(%d) : %s : %s\n"
               ,__LIBC_DEBUG_FILE,__LIBC_DEBUG_LINE,__LIBC_DEBUG_FUNC,expr);
 } else {
  ASSERT_PRINTF("    %s\n",expr);
 }
 ASSERT_PRINTF("===================================================\n");
 if (fmt) {
  va_list args;
  va_start(args,fmt);
  ASSERT_VPRINTF(fmt,args);
  va_end(args);
  ASSERT_PRINTF("\n");
 }
 ASSERT_PRINTF("===================================================\n");
#ifdef __DEBUG__
 tb_printex(skip+1);
#endif
#ifdef __KERNEL__
 {
  struct ktask *task = ktask_self();
  struct kproc *proc = task ? ktask_getproc(task) : NULL;
  struct kpagedir *pd = proc ? kproc_getpagedir(proc) : NULL;
  if (pd) kpagedir_print(pd);
  else ASSERT_PRINTF("NO PAGE DIRECTORY (task: %p, proc: %p, pd: %p)\n",task,proc,pd);
 }
 arch_hang();
 __builtin_unreachable();
#else
 abort();
#endif
}

#ifdef __KERNEL__
#ifdef KLOG_RAW
#   define PANIC_PRINTF(...)           (k_syslogf(KLOG_RAW,__VA_ARGS__),tty_printf(__VA_ARGS__))
#   define PANIC_VPRINTF(format,args) (k_vsyslogf(KLOG_RAW,format,args),tty_vprintf(format,args))
#else
#   define PANIC_PRINTF(...)           (serial_printf(SERIAL_01,__VA_ARGS__),tty_printf(__VA_ARGS__))
#   define PANIC_VPRINTF(format,args) (serial_vprintf(SERIAL_01,format,args),tty_vprintf(format,args))
#endif
void k_syspanic(__LIBC_DEBUG_PARAMS_ char const *fmt, ...) {
 karch_irq_disable();
#ifdef __x86__ // (Light) red screen!
 tty_x86_setcolors(vga_entry_color(VGA_COLOR_WHITE,VGA_COLOR_LIGHT_RED));
#endif
 tty_clear();
 PANIC_PRINTF("===================================================\n"
              " --- PANIC ---\n");
 if (__LIBC_DEBUG_FILE || __LIBC_DEBUG_FUNC) {
  PANIC_PRINTF("    " KDEBUG_SOURCEPATH_PREFIX "%s(%d) : %s\n"
              ,__LIBC_DEBUG_FILE,__LIBC_DEBUG_LINE,__LIBC_DEBUG_FUNC);
 }
 PANIC_PRINTF("===================================================\n");
 if (fmt) {
  va_list args;
  va_start(args,fmt);
  PANIC_VPRINTF(fmt,args);
  va_end(args);
  PANIC_PRINTF("\n");
 }
 PANIC_PRINTF("===================================================\n");
#ifdef __DEBUG__
 tb_printex(1);
#endif
 {
  struct ktask *task = ktask_self();
  struct kproc *proc = task ? ktask_getproc(task) : NULL;
  struct kpagedir *pd = proc ? kproc_getpagedir(proc) : NULL;
  if (pd) kpagedir_print(pd);
  else ASSERT_PRINTF("NO PAGE DIRECTORY (task: %p, proc: %p, pd: %p)\n",task,proc,pd);
 }
 arch_hang();
 __builtin_unreachable();
}
#endif


#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif
 
__public uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
__public __noreturn __noinline void __stack_chk_fail(void) {
 __assertion_failedf(NULL,-1,NULL,"STACK VIOLATION",1,NULL);
}


__DECL_END
#endif /* __DEBUG__ */

#endif /* !__ASSERT_C__ */
