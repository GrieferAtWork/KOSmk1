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
#ifndef __MOD_C__
#define __MOD_C__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <mod.h>
#include <kos/mod.h>
#include <errno.h>
#include <stddef.h>

__DECL_BEGIN

__public mod_t mod_open(char const *name) {
 mod_t result; kerrno_t error;
 error = kmod_open(name,(size_t)-1,&result,KMOD_OPEN_FLAG_NONE);
 if __likely(KE_ISOK(error)) return result;
 __set_errno(-error);
 return MOD_ERR;
}
__public mod_t mod_fopen(int fd) {
 mod_t result; kerrno_t error;
 error = kmod_fopen(fd,&result,KMOD_OPEN_FLAG_NONE);
 if __likely(KE_ISOK(error)) return result;
 __set_errno(-error);
 return MOD_ERR;
}
__public int mod_close(mod_t mod) {
 kerrno_t error;
 error = kmod_close(mod);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public void *mod_sym(mod_t mod, char const *symname) {
 void *result = kmod_sym(mod,symname,(size_t)-1);
 if __unlikely(!result) __set_errno(ENOENT);
 return result;
}


__DECL_END
#endif /* !__KERNEL__ */

#endif /* !__MOD_C__ */
