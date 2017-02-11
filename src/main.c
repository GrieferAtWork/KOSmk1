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
#ifndef __MAIN_C__
#define __MAIN_C__ 1

//
// === KOS ===
//

// BUILD REQUIREMENTS:
//  - GCC cross-compiler build for i386 (No special modifications needed)
//    - step-by-step instructions here: http://wiki.osdev.org/GCC_Cross-Compiler
//    - NOTE: I'm using GCC 6.2.0, but older versions should work, too...
//  - deemon (https://github.com/GrieferAtWork/deemon)
//    - If the latest commit doesn't work, use this one:
//      https://github.com/GrieferAtWork/deemon/commit/e9c85beb31e1e3263937c40b4a6f47ca70f54cf3
//  - QEMU
// NOTE: Yes, KOS doesn't have what would technically be considered a toolchain.
//       Instead, everything is done through magic(.dee)
// 
// SETUP:
//  - Potentially need to edit '/magic.dee' to use correct paths for compilers
//  - Though kind-of strange, I use Visual Studio as IDE and windows as
//    host platform, with it being possible that compilation in another
//    environment than my (admittedly _very_ strange) one may require
//    additional steps to be taken.
//
// BUILD+UPDATE+LINK+RUN:
//  :/$ deemon magic.dee
//  
//
// SUPPORTED:
//  - i386 (Correct types are always used; x86-64 support is planned)
//  - QEMU (Real hardware, or other emulators have not been tested (yet))
//  - multiboot (GRUB)
//  - dynamic memory (malloc)
//  - syscall
//  - multitasking/scheduler
//    - pid/tid
//    - low-cpu/true idle
//    - process
//      - argc|argv
//      - environ
//      - fork
//      - exec
//      - pipe (Missing named pipes)
//      - sandbox-oriented security model (barriers)
//      - shared memory (Missing from user-space)
//        - Reference-counted memory (Missing copy-on-write)
//      - mmap/munmap/brk/sbrk
//    - threads
//      - new_thread
//      - terminate
//      - suspend/resume
//      - priorities
//      - names
//      - detach/join
//      - yield
//      - alarm/pause
//      - tls
//    - synchronization primitives (Missing from user-space)
//      - semaphore
//      - mutex
//      - rwlock
//      - spinlock
//      - mmutex (non-standard primitive)
//      - signal+vsignal (non-standard primitive)
//      - addist+ddist (non-standard primitive)
//  - ELF binaries
//  - dlopen (Shared libraries)
//  - time (time_t/struct timespec)
//  - ring #3
//  - Filesystem
//    - open/read/write/seek/etc.
//    - OK:      Read/write existing files
//    - MISSING: create/delete/rename files/folders
//    - VFS (Virtual filesystem)
//      - OK:      /dev (incomplete)
//      - MISSING: /proc, /sys
//  - Highly posix-compliant
//  - PS/2 keyboard input
//  - ANSI-compliant Terminal
//  - User-space commandline (/bin/sh)
//
// UNSUPPORTED:
//  - High-precision timers
//  - FPU state saving

#include <assert.h>
#include <kos/errno.h>
#include <kos/kernel/dev/socket.h>
#include <kos/kernel/fpu.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/vfs.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/keyboard.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/task.h>
#include <kos/kernel/time.h>
#include <kos/kernel/tty.h>
#include <kos/syslog.h>
#include <stddef.h>
#include <stdlib.h>


void lib_test(void) {
 char const *argv[] = {NULL,NULL};
 char const *path = getenv("init");
 if (!path)  path = "/bin/init";
 //if (!path)  path = "/dev/tty";
 argv[0] = path;
 kerrno_t error; struct kshlib *lib;
 struct kproc *process; struct ktask *root;
 kmodid_t modid;
 // Create a new root process
 process = kproc_newroot();
 assert(process);
 asserte(KE_ISOK(kproc_locks(process,KPROC_LOCK_SHM|KPROC_LOCK_MODS)));
 error = kpagedir_mapkernel(process->p_shm.sm_pd,PAGEDIR_FLAG_READ_WRITE);
 assertf(KE_ISOK(error),"%d",error);
 error = kshlib_openfile(path,(size_t)-1,&lib);
 assertf(KE_ISOK(error),"%d",error);
 error = kproc_insmod_unlocked(process,lib,&modid);
 assertf(KE_ISOK(error),"%d",error);
 root  = kshlib_spawn(lib,process); 
 assert(root);
 kproc_unlocks(process,KPROC_LOCK_SHM|KPROC_LOCK_MODS);
 error = kproc_setenv_k(process
                       ,"SECRECT_KERNEL_VARIABLE",(size_t)-1
                       ,"You have uncovered the secret!",(size_t)-1
                       ,0);
 assertf(KE_ISOK(error),"%d",error);
 error = kprocenv_setargv(&process->p_environ,(size_t)-1,argv,NULL);
 assertf(KE_ISOK(error),"%d",error);
 error = ktask_setname(root,path);
 assertf(KE_ISOK(error),"%d",error);
 error = ktask_resume_k(root);
 assertf(KE_ISOK(error),"%d",error);
 //printf("Joining process\n");
 error = ktask_join(root,NULL);
 assertf(KE_ISOK(error),"%d",error);
 ktask_decref(root);
 kproc_decref(process);
 kshlib_decref(lib);
 // TODO: Implement recursive join and remove the following line!
 while (ktask_zero()->t_children.t_taska) ktask_yield();
}

int k_sysloglevel = KLOG_INFO;


static void *task_main(void *c) {
 struct timespec tm = {1,0};
 ktask_sleep(ktask_self(),&tm);
 return c;
}

void test_taskstat(void) {
 struct ktaskstat stat;
 struct ktask *task = ktask_new(&task_main,NULL);
 ktask_join(task,NULL);
 ktask_getstat(task,&stat);
 printf("Runtime: %I32u:%lu\n",stat.ts_reltime_run.tv_sec,stat.ts_reltime_run.tv_nsec);
 printf("Nosched: %I32u:%lu\n",stat.ts_reltime_nosched.tv_sec,stat.ts_reltime_nosched.tv_nsec);
 printf("Sleep:   %I32u:%lu\n",stat.ts_reltime_sleep.tv_sec,stat.ts_reltime_sleep.tv_nsec);
 ktask_decref(task);
}

void kernel_main(void) {

 // TODO: Parse explicit environ input in exec
 // TODO: Get away from a text-based GUI
 // TODO: Kernel modules
 // TODO: Create a /proc file system
 
 // TODO: Add support for deleting files.
 // TODO: Intrinsic support for wiping patterns.
 // TODO: Wiping pattern: https://youtu.be/NG9Cg_vBKOg?t=4m5s

 // TODO: Fix the mess that is user-kernel pointer translation.
 //      !YOU CAN'T JUST CONVERT A USER-POINTER TO KERNEL-SPACE!
 //    >> The idea of doing so completely clashes with with the
 //       process of what scattering memory does!
 //    >> Just like the linux kernel, we too must _only_ use our
 //       own versions of copy_to_user/copy_from_user/copy_in_user!
 // HINT: The way exec does it now is how it's safe to do.

 kernel_initialize_tty();
 kernel_initialize_serial();
 kernel_initialize_raminfo(); /*< NOTE: Also initializes arguments/environ. */
 kernel_initialize_gdt();
 kernel_initialize_interrupts();
 kernel_initialize_keyboard();
 kernel_initialize_process();
 kernel_initialize_paging();
 kernel_initialize_fpu();
 kernel_initialize_cmos();
 kernel_initialize_filesystem();
 kernel_initialize_vfs();
 kernel_initialize_syscall();
 assert(!karch_irq_enabled());
 karch_irq_enable();

 //setenv("LD_LIBRARY_PATH","/usr/lib:/lib",0);
 //setenv("PATH","/bin:/usr/bin:/usr/local/bin",0);
 //setenv("USER","kernel",0);
 //setenv("HOME","/",0);


#define RUN(x) extern void x(void); x()
 //RUN(test_interrupts);
 //RUN(test_tasks);
 //RUN(test_signals);
 //RUN(test_sleep_terminate);
 //RUN(test_detachsuspended);
 //RUN(test_terminate_suspended_critical);
#undef RUN
 //test_taskstat();

 //_malloc_validate_d();
 //__u8 *p = (__u8 *)malloc(1);
 //_malloc_validate_d();
 //p[-1] = 42;
 //_malloc_validate_d();
 //free(p);
 //_malloc_validate_d();

 lib_test();

 karch_irq_disable();
 kernel_finalize_process();
 kernel_finalize_filesystem();
 ketherframe_clearcache();
 _malloc_printleaks_d();

 if (getenv("hang")) for (;;) {
  karch_irq_disable();
  karch_irq_idle();
 }
}

#endif /* !__MAIN_C__ */
/*
target remote xage:1234
file /kos/bin/kos_kernel.bin
y
continue
*/
