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
#ifndef __LIB_DISASM_H__
#define __LIB_DISASM_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include <stdint.h>
#include <stddef.h>
#include <format-printer.h>

__DECL_BEGIN


#define DISASM_FLAG_NONE 0x00000000
#define DISASM_FLAG_ADDR 0x00000001 /*< Print the address offset on line starts. */

extern int disasm_x86(void const *__restrict text, size_t text_size,
                      pformatprinter printer, void *closure,
                      uint32_t flags);



__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__LIB_DISASM_H__ */
