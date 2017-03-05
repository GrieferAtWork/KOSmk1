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
#ifndef __KOS_KERNEL_KEYBOARD_H__
#define __KOS_KERNEL_KEYBOARD_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/keyboard.h>
#include <kos/kernel/types.h>
#include <kos/errno.h>
#include <kos/kernel/addist.h>

__DECL_BEGIN

extern void kernel_initialize_keyboard(void);

//////////////////////////////////////////////////////////////////////////
// Data distributer for keyboard input.
// WARNING: Receiving a signal from the keyboard will put the kernel
//          into a state appearing as though it was a deadlock.
//          Therefor, you must wrap calls to recv() within a
//          'ktask_deadlock_intended_begin()..ktask_deadlock_intended_end()'
//          block.
// @value_type: struct kbevent.
extern struct kaddist keyboard_input;

#define KEYBOARD_GENTICKET(ticket)  kaddist_genticket(&keyboard_input,ticket)
#define KEYBOARD_DELTICKET(ticket)  kaddist_delticket(&keyboard_input,ticket)
#define KEYBOARD_RECV(ticket,event) KTASK_DEADLOCK_INTENDED(kaddist_vrecv(&keyboard_input,ticket,event))

extern struct kkeymap const *kkeymap_current;

extern void keyboard_sendscan(kbscan_t scan);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_KEYBOARD_H__ */
