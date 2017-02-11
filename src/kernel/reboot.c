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
#ifndef __KOS_KERNEL_REBOOT_C__
#define __KOS_KERNEL_REBOOT_C__ 1

#include <kos/compiler.h>
#include <kos/arch/irq.h>
#include <sys/io.h>
#include <kos/kernel/tty.h>
#ifdef __x86__
#include <kos/arch/x86/vga.h>
#endif

void hw_reboot(void) {
 karch_irq_disable();
 while (inb(0x64)&0x02);
 outb(0x64,0xFE);
 for (;;) {
  karch_irq_disable();
  karch_irq_idle();
 }
}

void hw_shutdown(void) {
 karch_irq_disable();
 outb(0xf4,0x00); // This can be used to shutdown QEMU (potentially...)
 {
  const char *s = "Shutdown";
  for (; *s; ++s) outb(0x8900,*s);
 }
 // TODO: ACPI shutdown

#ifdef __x86__
 tty_x86_setcolors(vga_entry_color(VGA_COLOR_WHITE,VGA_COLOR_LIGHT_GREY));
#endif
 tty_clear();
 tty_print("\n"
           "\n"
           "\n"
           "\n"
           "\n"
           "\n" // I can't do it... I can't pull the trigger myself...
           "\n" // You do it!
           "\n"
           "                    It is now safe to turn off your computer"
           );
 for (;;) {
  karch_irq_disable();
  karch_irq_idle();
 }
 __builtin_unreachable();
}

#endif /* !__KOS_KERNEL_REBOOT_C__ */
