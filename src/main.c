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
#include <kos/kernel/cpu.h>
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
#include <kos/kernel/shm.h>
#include <kos/kernel/task.h>
#include <kos/kernel/time.h>
#include <kos/kernel/tty.h>
#include <kos/kernel/syslog_io.h>
#include <kos/syslog.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <kos/compiler.h>
#include <kos/kernel/panic.h>

STATIC_ASSERT(sizeof(__s8)  == 1);
STATIC_ASSERT(sizeof(__u8)  == 1);
STATIC_ASSERT(sizeof(__s16) == 2);
STATIC_ASSERT(sizeof(__u16) == 2);
STATIC_ASSERT(sizeof(__s32) == 4);
STATIC_ASSERT(sizeof(__u32) == 4);
STATIC_ASSERT(sizeof(__s64) == 8);
STATIC_ASSERT(sizeof(__u64) == 8);


void run_init(void) {
 char const *argv[] = {NULL,NULL};
 char const *path = getenv("init");
 if (!path)  path = "/bin/init";
 argv[0] = path;
 kerrno_t error; struct kshlib *lib;
 struct kproc *process; struct ktask *root;
 kmodid_t modid; void *exitcode;
 // Create a new root process
 process = kproc_newroot();
 assert(process);
 asserte(KE_ISOK(kproc_locks(process,KPROC_LOCK_MODS)));
 asserte(KE_ISOK(krwlock_beginwrite(&process->p_shm.s_lock)));
 error = kshlib_openexe(path,(size_t)-1,&lib,KTASK_EXEC_FLAG_NONE);
 assertf(KE_ISOK(error),"%d",error);
 error = kproc_insmod_unlocked(process,lib,&modid);
 assertf(KE_ISOK(error),"%d",error);
 root  = kshlib_spawn(lib,process); 
 assert(root);
 krwlock_endwrite(&process->p_shm.s_lock);
 kproc_unlocks(process,KPROC_LOCK_MODS);
 error = kproc_setenv_k(process
                       ,"SECRECT_KERNEL_VARIABLE",(size_t)-1
                       ,"You have uncovered the secret!",(size_t)-1
                       ,0);
 assertf(KE_ISOK(error),"%d",error);
 error = kprocenv_setargv(&process->p_environ,(size_t)-1,argv,NULL);
 assertf(KE_ISOK(error),"%d",error);
 error = ktask_setname(root,"INIT");
 assertf(KE_ISOK(error),"%d",error);
 ksyslog_deltty();
 error = ktask_resume_k(root);
 assertf(KE_ISOK(error),"%d",error);
 //printf("Joining process\n");
 error = ktask_join(root,&exitcode);
 assertf(KE_ISOK(error),"%d",error);
 ktask_decref(root);
 kproc_decref(process);
 kshlib_decref(lib);
 // TODO: Implement recursive join and remove the following line!
 while (ktask_zero()->t_children.t_taska) ktask_yield();
 ;
 k_syslogf(KLOG_INFO,"[init] Joined root process returning %Iu\n",exitcode);
}

int k_sysloglevel = KLOG_DEBUG;
//int k_sysloglevel = KLOG_TRACE;


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
 ktime_getnow(&start);
 allocs = (void **)malloc(n_allocs*sizeof(void *));
 for (i = 0; i < n_allocs; ++i) allocs[i] = malloc(alloc_each);
 ktime_getnow(&before_free);
 for (i = 0; i < n_allocs; ++i) free(allocs[i]);
 free(allocs);
 ktime_getnow(&stop);
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

#include <hw/net/ne2000.h>
#include <kos/arch/x86/bios.h>
#include <kos/kernel/arch/x86/realmode.h>
#include <hw/video/vga.h>

#include <kos/arch/x86/string.h>

void test_vga(void) {
 struct realmode_regs regs;
 struct vgastate mode;

 memset(&regs,0,sizeof(regs));
 regs.ah = BIOS_VGA_SETMODE;
 regs.al = BIOS_VGA_MODE_TEXT_40x25x16_GRAY;
 regs.al = BIOS_VGA_MODE_GFX_640x480x16;
 regs.al = BIOS_VGA_MODE_TEXT_80x25x16_COLOR;
 regs.al = BIOS_VGA_MODE_GFX_320x200x256;
 realmode_interrupt(BIOS_INTNO_VGA,&regs);

 unsigned pitch = 320;
 unsigned sx = 320;
 unsigned sy = 200;
 char *vid = (char *)0xA0000;
 for (int y = 0; y < sy; ++y) {
  for (int x = 0; x < sx; ++x) {
   vid[x] = (x+y) % 256;
  }
  vid += pitch;
 }

 vgastate_save(&mode);
 serial_printf(SERIAL_01,"misc          = %x\n",mode.vs_mis_regs.vm_misc);
 serial_printf(SERIAL_01,"pitch         = %u\n",vgastate_get_pitch(&mode));
 serial_printf(SERIAL_01,"dim.x         = %u\n",vgastate_get_dim_x(&mode));
 serial_printf(SERIAL_01,"dim.y         = %u\n",vgastate_get_dim_y(&mode));
 serial_printf(SERIAL_01,"h_total       = %u\n",vgastate_get_crt_h_total      (&mode));
 serial_printf(SERIAL_01,"h_disp        = %u\n",vgastate_get_crt_h_disp       (&mode));
 serial_printf(SERIAL_01,"h_blank_start = %u\n",vgastate_get_crt_h_blank_start(&mode));
 serial_printf(SERIAL_01,"h_sync_start  = %u\n",vgastate_get_crt_h_sync_start (&mode));
 serial_printf(SERIAL_01,"h_blank_end   = %u\n",vgastate_get_crt_h_blank_end  (&mode));
 serial_printf(SERIAL_01,"h_sync_end    = %u\n",vgastate_get_crt_h_sync_end   (&mode));
 serial_printf(SERIAL_01,"h_skew        = %u\n",vgastate_get_crt_h_skew       (&mode));
 serial_printf(SERIAL_01,"v_total       = %u\n",vgastate_get_crt_v_total      (&mode));
 serial_printf(SERIAL_01,"v_sync_start  = %u\n",vgastate_get_crt_v_sync_start (&mode));
 serial_printf(SERIAL_01,"v_sync_end    = %u\n",vgastate_get_crt_v_sync_end   (&mode));
 serial_printf(SERIAL_01,"v_disp_end    = %u\n",vgastate_get_crt_v_disp_end   (&mode));
 serial_printf(SERIAL_01,"v_blank_start = %u\n",vgastate_get_crt_v_blank_start(&mode));
 serial_printf(SERIAL_01,"v_blank_end   = %u\n",vgastate_get_crt_v_blank_end  (&mode));
 //asserte(KE_ISOK(vgastate_set_crt_v_disp_end(&mode,400)));
 vgastate_restore(&mode);

 //memset((char *)0xA0000,0x0f,320*200);
 getchar();

 /* switch to 80x25x16 text mode */
 regs.ah = BIOS_VGA_SETMODE;
 regs.al = BIOS_VGA_MODE_TEXT_80x25x16_COLOR;
 realmode_interrupt(BIOS_INTNO_VGA,&regs);
}

static __attribute_used __u8 second_core_stack[4096];
extern __u8 second_core_bootbegin[];
extern __u8 second_core_bootend[];
extern struct kidtpointer sc_gdt;

#define SC_START   0x7C00 /* Abuse realmode for now... */
#define SC_SYM(x) __PP_STR((((x)-second_core_bootbegin)+SC_START))

__asm__(".code16\n"
        "second_core_bootbegin:\n"
        "    cli\n" /* Just in case... */
        "    lgdt (" SC_SYM(sc_gdt) ")\n"
        /* Enter protected mode. */
        "    mov %cr0, %eax\n"
        "    or  $0x00000001, %eax\n"
        "    mov %eax, %cr0\n"
        "    ljmpl $" __PP_STR(KSEG_KERNEL_CODE) ", $second_core_hibegin\n"
        "sc_gdt:\n"
        "    .word	0\n"
        "    .long	0\n"
        "second_core_bootend:\n"
        ".size second_core_bootbegin, . - second_core_bootbegin\n"
        ".size second_core_bootend, 0\n"
        ".code32\n"

        "second_core_hibegin:\n"
        "    mov $(" __PP_STR(KSEG_KERNEL_DATA) "), %ax\n"
        "    mov %ax, %ds\n"
        "    mov %ax, %es\n"
        "    mov %ax, %fs\n"
        "    mov %ax, %gs\n"
        "    mov %ax, %ss\n"
        "    lea (second_core_stack+4096), %esp\n"
        "    jmp second_core_main\n"
        ".size second_core_hibegin, . - second_core_hibegin\n"
        "\n"
);
static __attribute_used void second_core_main(void) {
 cpu_acknowledge();
 for (;;) {
  k_syslogf(KLOG_INFO,"[CPU #%d] Here...\n",cpu_self());
 }
 cpu_stopme();
}


void smp_test(void) {
 cpuid_t id; uintptr_t sc_eip; kerrno_t error;
 if (kcpu2_c <= 1) return;
 id = cpu_unused();
 assert(id != CPUID_INVALID);
 kpaging_disable();
 __asm_volatile__("sgdt (sc_gdt)\n");
 memcpy((void *)SC_START,second_core_bootbegin,
        (uintptr_t)second_core_bootend-
        (uintptr_t)second_core_bootbegin);
 sc_eip = (uintptr_t)SC_START;
 error = cpu_start(id,sc_eip);
 assert(KE_ISOK(error));
 for (;;) {
  k_syslogf(KLOG_INFO,"[CPU #%d] Here...\n",cpu_self());
 }
}



void kernel_main(void) {

 // TODO: Every CPU needs its own entry in the GDT as to allow
 //       for an easy and efficient mapping of per-cpu memory
 //       regions, similar to how the the new TLS engine works.
 // TODO: Rewrite the scheduler to make use of SMP, and also
 //       remove the chance of a task randomly being moved to
 //       a different CPU, as was planned until now.
 //       With these changes, ktask_self() can be simplified to one instruction:
 //    >> __asm__("mov (KCPU_OFFSETOF_CURRENT)(%%{fs|gs}), %0" : "=r" (ktask_self));
 //       NOTE: The segment register used should mirror that for TLS

 // TODO: When switching to a kernel stack from usermode, instead of
 //       re-calculating the physical address of the stack (since the
 //       usermode page directory will still be set), pre-calculate
 //       that address and store it at the base of the stack.
 //    >> When switching to kernel-mode, the virtual address of the
 //       kernel stack will be set properly, and instead of having
 //       to do that long and complicated 'TRANSLATE_VADDR'-macro,
 //       the entire address-space switch could be simlified to:
 //    >> mov (SIZEOF_STORED_USER_REGISTERS)(%esp), %esp /*< Load the physical ESP, as stored ontop of the kernel stack */
 //    >> lea __kpagedir_kernel, %eax                    /*< Switch to the kernel page directory. */
 //    >> mov %eax, %cr3                                 /*< *ditto*. */

 // TODO: Parse explicit environ input in exec
 // TODO: Get away from a text-based GUI
 // TODO: Kernel modules
 // TODO: Create a /proc file system
 // TODO: ELF thread-local memory

 // TODO: Fix the mess that is user-kernel pointer translation.
 //      !YOU CAN'T JUST CONVERT A USER-POINTER TO KERNEL-SPACE!
 //    >> The idea of doing so completely clashes with with the
 //       process of what scattering memory does!
 //    >> Just like the linux kernel, we too must _only_ use our
 //       own versions of copy_to_user/copy_from_user/copy_in_user!
 // HINT: The way exec does it now is how it's safe to do.

 kernel_initialize_time();
 kernel_initialize_hpet();
 kernel_initialize_tty();
 kernel_initialize_serial();
 kernel_initialize_raminfo(); /*< NOTE: Also initializes SMP and arguments/environ. */
 ksyslog_addtty();
 kernel_initialize_syslog();
 kernel_initialize_realmode();
 kernel_initialize_interrupts();
 kernel_initialize_gdt();
 kernel_initialize_paging();
 kernel_initialize_keyboard();
 kernel_initialize_fpu();
 kernel_initialize_copyonwrite();
 kernel_initialize_process();
 kernel_initialize_filesystem();
 kernel_initialize_vfs();
 kernel_initialize_syscall();
 kernel_initialize_vga();
 assert(!karch_irq_enabled());



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
 //test_vga();
 //smp_test();
 run_init();

 ksyslog_deltty();
 karch_irq_disable();
 kernel_finalize_process();
 kshlibrecent_clear(); /*< Must clear before filesystem to close open library files. */
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
