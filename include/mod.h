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
#ifndef __MOD_H__
#define __MOD_H__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <kos/compiler.h>
#include <kos/mod.h>

#ifdef __STDC_PURE__
#warning "This header is only available on KOS platforms."
#endif

//////////////////////////////////////////////////////////////////////////
// GLibc - style wrapper functions for KOS module/shared_lib/dynlib system calls.
// >> Functions behave the same as their <kos/mod.h> counterparts,
//    though they are integrated into the errno system and return
//    -1 on error, and 0 (or some other, meaningful value) on success.
// >> Detailed documentations can be found in <kos/mod.h>

__DECL_BEGIN

#define MOD_ALL  KMODID_ALL  /*< Search for symbols in all loaded modules. */
#define MOD_NEXT KMODID_NEXT /*< Search for symbols in all loaded modules, checking the calling module last. */
#define MOD_SELF KMODID_SELF /*< Search for symbols in the calling module. */
#define MOD_KERN KMODID_KERN /*< Reserved for future use: Special kernel-based module containing statically loaded user-space helper functions. */

#define MOD_ERR  ((kmodid_t)-5)

#ifndef __mod_t_defined
#define __mod_t_defined 1
typedef kmodid_t mod_t;
#endif

//////////////////////////////////////////////////////////////////////////
extern __wunused __nonnull((1)) mod_t mod_open  __P((char const *__restrict __name));
extern __wunused                mod_t mod_fopen __P((int __fd));
extern                          int   mod_close __P((mod_t __mod));
extern __wunused __nonnull((2)) void *mod_sym   __P((mod_t __mod, char const *__restrict __symname));

__DECL_END
#endif /* !__KERNEL__ */

#endif /* !__MOD_H__ */
