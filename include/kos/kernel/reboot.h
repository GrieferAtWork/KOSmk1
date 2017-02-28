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
#ifndef __KOS_KERNEL_REBOOT_H__
#define __KOS_KERNEL_REBOOT_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Perform a low-level hardware-based reboot/shutdown.
// WARNING: Once these are called, there is no turning back,
//          nor any way of previously registering some hooks
//          to-be called before commencing.
//       >> Only call these if you're absolutely
//          sure that that's what you want to do.
extern __noreturn void hw_reboot(void);
extern __noreturn void hw_shutdown(void);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_REBOOT_H__ */
