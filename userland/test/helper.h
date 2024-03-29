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


#include <kos/syslog.h>
#include <stdio.h>
#include <stdarg.h>

#define NAME(name) _test__##name
#define TEST(name) __public void NAME(name)(void)

static __attribute_unused void outf(char const *fmt, ...) {
 va_list args;
 va_start(args,fmt);
 k_vsyslogf(KLOG_INFO,fmt,args);
 va_end(args);
 va_start(args,fmt);
 vprintf(fmt,args);
 va_end(args);
}


#include <assert.h>
#include <string.h>
#include <errno.h>
#define ASSERT_ERRNO(expr) \
 { errno = 0; if (!(expr)) assertf(0,"Operation " #expr " failed: %d: %s",errno,strerror(errno)); }



