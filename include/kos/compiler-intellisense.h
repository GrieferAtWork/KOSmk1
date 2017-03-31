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

#ifndef __likely
#define __likely /* nothing */
#endif
#ifndef __unlikely
#define __unlikely /* nothing */
#endif

#ifndef __xreturn
#define __xreturn         return
#endif
#ifndef __xblock
// NOTE: Don't worry. This is not meant for generated code, but
//       simply to get correct syntax highlighting in visual studio...
#define __xblock(...)     (([&]__VA_ARGS__)())
#endif
#ifndef __asm_goto__
#define __asm_goto__      __asm__
#endif
#ifndef __asm_volatile__
#define __asm_volatile__  __asm__
#endif

#ifndef __compiler_alignof
#define __compiler_alignof   __alignof
#endif
#ifndef __compiler_pragma
#define __compiler_pragma       __pragma
#endif
#ifndef __COMPILER_PACK_PUSH
#define __COMPILER_PACK_PUSH(n) __pragma(pack(push,n))
#define __COMPILER_PACK_POP     __pragma(pack(pop))
#endif
