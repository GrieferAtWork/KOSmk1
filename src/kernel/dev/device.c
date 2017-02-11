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
#ifndef __KOS_KERNEL_DEV_DEVICE_C__
#define __KOS_KERNEL_DEV_DEVICE_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/dev/device.h>
#include <malloc.h>

__DECL_BEGIN

extern void kdev_destroy(struct kdev *self);
void kdev_destroy(struct kdev *self) {
 kassert_object(self,KOBJECT_MAGIC_DEV);
 // Close the device (if the mutex was already closed, skip that part)
 if (krwlock_close(&self->d_rwlock) != KS_UNCHANGED &&
     self->d_quit) (*self->d_quit)(self);
 free(self);
}

__DECL_END

#ifndef __INTELLISENSE__
#include "storage.c.inl"
#endif

#endif /* !__KOS_KERNEL_DEV_DEVICE_C__ */
