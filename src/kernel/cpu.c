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
#ifndef __KOS_KERNEL_CPU_C__
#define __KOS_KERNEL_CPU_C__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/kernel/cpu.h>
#include <kos/kernel/multiboot.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <kos/syslog.h>

__DECL_BEGIN

extern multiboot_info_t *__grub_mbt;
extern unsigned int      __grub_magic;


struct mpfps {
 /* mp_floating_pointer_structure */
    char signature[4];
    uint32_t configuration_table;
    uint8_t length; // In 16 bytes (e.g. 1 = 16 bytes, 2 = 32 bytes)
    uint8_t mp_specification_revision;
    uint8_t checksum; // This value should make all bytes in the table equal 0 when added together
    uint8_t default_configuration; // If this is not zero then configuration_table should be 
                                   // ignored and a default configuration should be loaded instead
    uint32_t features; // If bit 7 is then the IMCR is present and PIC mode is being used, otherwise 
                       // virtual wire mode is; all other bits are reserved
};


static struct mpfps *
mpfps_locate_at(void *base, size_t bytes) {
#define MP_ALIGN 16
 static char const sig[] = {'_','M','P','_'};
 struct mpfps *result = NULL;
 uintptr_t iter,end;
 end = (iter = (uintptr_t)base)+bytes;
 for (; iter != end; ++iter) {
  /* Search for the signature. */
  result = (struct mpfps *)memidx((void *)alignd(iter,MP_ALIGN),
                                  ceildiv(end-iter,MP_ALIGN),
                                  sig,sizeof(sig),MP_ALIGN);
  if (!result) break;
  /* When found, check the checksum. */
  break;

  iter = (uintptr_t)result;
 }
 return result;
}

static struct mpfps *mpfps_locate(void) {
 struct mpfps *result;
              result = mpfps_locate_at((void *)(uintptr_t)*(__u16 *)0x40E,1024);
 if (!result) result = mpfps_locate_at((void *)(uintptr_t)((*(__u16 *)0x413)*1024),1024);
 if (!result) result = mpfps_locate_at((void *)(uintptr_t)0x0F0000,64*1024);
 return result;
}



void kernel_initialize_cpu(void) {
 struct mpfps *f = mpfps_locate();
 k_syslogf(KLOG_INFO,"Found MPFPS structure at %p\n",f);
}

__DECL_END

#endif /* !__KOS_KERNEL_CPU_C__ */
