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
#ifndef __SYS_SYSINFO_H__
#define __SYS_SYSINFO_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/sysconf.h>

__DECL_BEGIN

#ifndef __CONFIG_MIN_LIBC__
#ifdef _SC_PHYS_PAGES
extern long get_phys_pages __P((void));
#endif
#ifdef _SC_AVPHYS_PAGES
extern long get_avphys_pages __P((void));
#endif
#elif defined(__KERNEL__)
extern long k_sysconf(int name);
#ifdef _SC_PHYS_PAGES
#define get_phys_pages()   k_sysconf(_SC_PHYS_PAGES)
#endif
#ifdef _SC_AVPHYS_PAGES
#define get_avphys_pages() k_sysconf(_SC_AVPHYS_PAGES)
#endif
#endif /* ... */

__DECL_END

#endif /* !__SYS_SYSINFO_H__ */
