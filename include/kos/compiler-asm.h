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

#ifndef __compiler_FUNCTION
#define __compiler_FUNCTION()
#endif
#ifndef __noreturn
#define __noreturn
#endif
#ifndef __nonnull
#define __nonnull(ids)
#endif
#ifndef __noinline
#define __noinline
#endif
#ifndef __constcall
#define __constcall
#endif
#ifndef __malloccall
#define __malloccall
#endif
#ifndef __sizemalloccall
#define __sizemalloccall(xy)
#endif
#ifndef __wunused
#define __wunused
#endif
#ifndef __retnonnull
#define __retnonnull
#endif
#ifndef __packed
#define __packed
#endif
#ifndef __attribute_vaformat
#define __attribute_vaformat(func,fmt_idx,varargs_idx)
#endif
#ifndef __attribute_used
#define __attribute_used
#endif
#ifndef __attribute_unused
#define __attribute_unused
#endif
#ifndef __attribute_forceinline
#define __attribute_forceinline
#endif
#ifndef __attribute_inline
#define __attribute_inline
#endif

#ifndef __evalexpr
#define __evalexpr(x) x
#endif

#ifndef __likely
#define __likely(x)   x
#endif
#ifndef __unlikely
#define __unlikely(x) x
#endif

#ifndef __compiler_ALIAS
#define __compiler_ALIAS(new_name,old_name) \
 .global new_name; .type new_name, @function;\
 new_name: jmp old_name
#endif

#ifndef __compiler_UNIQUE
#ifdef __COUNTER__
#define __compiler_UNIQUE(group) __PP_CAT_3(__hidden_,group,__COUNTER__)
#endif
#endif /* !__compiler_UNIQUE */

#ifndef __STATIC_ASSERT
#define __STATIC_ASSERT(expr)
#endif

#ifndef __xreturn
#define __xreturn
#endif
#ifndef __xblock
#define __xblock
#endif
#ifndef __asm_goto__
#define __asm_goto__
#endif
#ifndef __asm_volatile__
#define __asm_volatile__
#endif
#ifndef __compiler_unreachable
#define __compiler_unreachable()
#endif
