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
#ifndef __KOS_ARCH_GENERIC_STRING_H__
#define __KOS_ARCH_GENERIC_STRING_H__ 1

#include <kos/compiler.h>

#ifdef __INTELLISENSE__
#include <kos/arch/x86/string.h>
#endif

#ifndef __ASSEMBLY__
/* Using low-level assembly functions implemented
 * by the #include-er, generate high-level wrappers
 * for string utilities, featuring constant
 * optimizations for arguments known at compile time.
 * NOTE: The functions in this file assume and
 *       implement stdc-compliant semantics.
 */
#include "string/ffs.h"
#include "string/memchr.h"
#include "string/memcmp.h"
#include "string/memcpy.h"
#include "string/memend.h"
#include "string/memidx.h"
#include "string/memlen.h"
#include "string/memmem.h"
#include "string/memrchr.h"
#include "string/memridx.h"
#include "string/memrmem.h"
#include "string/memset.h"

/* Helper macros for string routines. */
#define arch_strend(s)         (char *)arch_umemend(s,'\0')
#define arch_strlen(s)                 arch_umemlen(s,'\0')
#define arch_strnend(s,maxlen) (char *)arch_memend(s,'\0',maxlen)
#define arch_strnlen(s,maxlen)         arch_memlen(s,'\0',maxlen)

#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_H__ */
