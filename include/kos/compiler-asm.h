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
#define __compiler_FUNCTION() /* nothing */
#endif
#ifndef __noreturn
#define __noreturn /* nothing */
#endif
#ifndef __nonnull
#define __nonnull(ids) /* nothing */
#endif
#ifndef __noinline
#define __noinline /* nothing */
#endif
#ifndef __constcall
#define __constcall /* nothing */
#endif
#ifndef __malloccall
#define __malloccall /* nothing */
#endif
#ifndef __sizemalloccall
#define __sizemalloccall(xy) /* nothing */
#endif
#ifndef __wunused
#define __wunused /* nothing */
#endif
#ifndef __retnonnull
#define __retnonnull /* nothing */
#endif
#ifndef __packed
#define __packed /* nothing */
#endif
#ifndef __attribute_vaformat
#define __attribute_vaformat(func,fmt_idx,varargs_idx) /* nothing */
#endif
#ifndef __attribute_used
#define __attribute_used /* nothing */
#endif
#ifndef __attribute_unused
#define __attribute_unused /* nothing */
#endif
#ifndef __attribute_forceinline
#define __attribute_forceinline /* nothing */
#endif
#ifndef __attribute_inline
#define __attribute_inline /* nothing */
#endif
#ifndef __local
#define __local /* nothing */
#endif
#ifndef __forcelocal
#define __forcelocal /* nothing */
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

#ifndef __COMPILER_ALIAS
#define __COMPILER_ALIAS(new_name,old_name) \
 .global new_name; .type new_name, @function;\
 new_name: jmp old_name
#endif

#ifndef __COMPILER_UNIQUE
#ifdef __COUNTER__
#define __COMPILER_UNIQUE(group) __PP_CAT_3(__hidden_,group,__COUNTER__)
#endif
#endif /* !__COMPILER_UNIQUE */

#ifndef __STATIC_ASSERT
#define __STATIC_ASSERT(expr) /* nothing */
#endif

#ifndef __xreturn
#define __xreturn /* nothing */
#endif
#ifndef __xblock
#define __xblock /* nothing */
#endif
#ifndef __asm_goto__
#define __asm_goto__ /* nothing */
#endif
#ifndef __asm_volatile__
#define __asm_volatile__ /* nothing */
#endif
#ifndef __compiler_unreachable
#define __compiler_unreachable() /* nothing */
#endif

#undef __DECL_BEGIN
#undef __DECL_END
#define __DECL_BEGIN  /* never anything! */
#define __DECL_END    /* never anything! */

