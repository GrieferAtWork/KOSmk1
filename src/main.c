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


void run_init(void) {
 char const *argv[] = {NULL,NULL};
 char const *path = getenv("init");
 if (!path)  path = "/bin/init";
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

int k_sysloglevel = KLOG_DEBUG;


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

#if 0
void stress_malloc(void) {
 struct timespec start,before_free,stop;
 void **allocs;
 size_t i;
 size_t n_allocs = 1024*4;
 size_t alloc_each = 4097;
 ktime_getnoworcpu(&start);
 allocs = (void **)malloc(n_allocs*sizeof(void *));
 for (i = 0; i < n_allocs; ++i) allocs[i] = malloc(alloc_each);
 ktime_getnoworcpu(&before_free);
 for (i = 0; i < n_allocs; ++i) free(allocs[i]);
 free(allocs);
 ktime_getnoworcpu(&stop);
 __timespec_sub(&stop,&before_free);
 __timespec_sub(&before_free,&start);
 printf("MALLOC TIME: %I32u.%ld\n",stop.tv_sec,stop.tv_nsec);
 printf("FREE TIME: %I32u.%ld\n",before_free.tv_sec,before_free.tv_nsec);
}
#endif

void test_write_file(void) {
 kerrno_t error;
 error = krootfs_remove("/directory_with_a_really_long_name",(size_t)-1);
 assertf(KE_ISOK(error) || error == KE_NOENT,"%d",error);
 error = krootfs_mkdir("/directory_with_a_really_long_name",(size_t)-1,0,NULL,NULL,NULL);
 assertf(KE_ISOK(error),"%d",error);

#if 0
 struct kfile *fp;
 error = krootfs_remove("/test.txt",(size_t)-1);
 assertf(KE_ISOK(error) || error == KE_NOENT,"%d",error);

 error = krootfs_open("/test.txt",(size_t)-1,O_CREAT|O_EXCL|O_WRONLY,0,NULL,&fp);
 assertf(KE_ISOK(error),"%d",error);
 error = fprintf(fp,"This text should appear in the file!\n");
 assertf(KE_ISOK(error),"%d",error);
 error = fprintf(fp,"fp = %p\n",fp);
 assertf(KE_ISOK(error),"%d",error);
 kfile_decref(fp);
#endif
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

 //test_write_file();
 run_init();

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
