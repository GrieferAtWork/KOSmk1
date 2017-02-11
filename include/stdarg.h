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
#ifndef __STDARG_H__
#ifndef _STDARG_H
#ifndef _STDARG_H_
#ifndef _INC_STDARG
#define __STDARG_H__ 1
#define _STDARG_H 1
#define _STDARG_H_ 1
#define _INC_STDARG 1

#include <vadefs.h>

#define va_start   _crt_va_start  // void va_start(va_list &ap, T &last);
#define va_end     _crt_va_end    // void va_end(va_list &ap);
#define va_copy       __va_copy   // void va_end(va_list &dst_ap, va_list &src_ap);
#define va_arg     _crt_va_arg    // void va_end(va_list &ap, typename T);

#endif /* !_INC_STDARG */
#endif /* !_STDARG_H_ */
#endif /* !_STDARG_H */
#endif /* !__STDARG_H__ */
