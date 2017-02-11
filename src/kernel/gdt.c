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
#ifndef __KOS_KERNEL_GDT_C__
#define __KOS_KERNEL_GDT_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/task.h>
#include <kos/syslog.h>
#include <kos/kernel/tss.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/task.h>

__DECL_BEGIN

struct ksegment gdt_segments[6];


extern byte_t stack_top[];
extern void gdt_flush(struct kidtpointer *p);
extern void tss_flush(__u16 sel);

void kernel_initialize_gdt(void) {
 struct ktss *tss0 = &kcpu_zero()->c_tss;

 memset(tss0,0,sizeof(struct ktss));

 tss0->esp0 = 0; /*< Set when switching to user tasks. */
 tss0->ss0  = KSEGMENT_KERNEL_DATA;
/*
 tss0->cs         = KSEGMENT_KERNEL_CODE;
 tss0->ss         = KSEGMENT_KERNEL_DATA;
 tss0->ds         = KSEGMENT_KERNEL_DATA;
 tss0->es         = KSEGMENT_KERNEL_DATA;
 tss0->fs         = KSEGMENT_KERNEL_DATA;
 tss0->gs         = KSEGMENT_KERNEL_DATA;
 tss0->iomap_base = sizeof(struct ktss);
*/

 // Configure the GDT gates
 ksegment_encode(&gdt_segments[0],0,0,0);                        /* NULL segment */
 ksegment_encode(&gdt_segments[1],0,SEG_LIMIT_MAX,SEG_CODE_PL0); /* Kernel code segment */
 ksegment_encode(&gdt_segments[2],0,SEG_LIMIT_MAX,SEG_DATA_PL0); /* Kernel data segment */
 ksegment_encode(&gdt_segments[3],0,SEG_LIMIT_MAX,SEG_CODE_PL3); /* User code */
 ksegment_encode(&gdt_segments[4],0,SEG_LIMIT_MAX,SEG_DATA_PL3); /* User data */
 ksegment_encode(&gdt_segments[5],(uintptr_t)tss0,sizeof(struct ktss),SEG_TSS); /* Kernel TSS */

 // Configure the IDT Pointer
 struct kidtpointer idt_pointer; /*< TODO: This one may not have to be global. */
 idt_pointer.base  = gdt_segments;
 idt_pointer.limit = sizeof(gdt_segments)-1;

 assertf(!kpaging_enabled(),"Must initialize GDT before paging");
 gdt_flush(&idt_pointer); // Install the GDT table (overwriting the one set by GRUB)
 assert(gdt_segments[5].rw == 0);
 tss_flush(KSEGMENT_KERNELTSS|3);    // Set the TSS segment
 assert(gdt_segments[5].rw == 1);
}

__DECL_END

#endif /* !__KOS_KERNEL_GDT_C__ */
