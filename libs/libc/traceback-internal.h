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
#ifndef __TRACEBACK_INTERNAL_H__
#define __TRACEBACK_INTERNAL_H__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <traceback.h>

__DECL_BEGIN

struct dtracebackentry {
 void const *tb_eip;
 void const *tb_esp;
};
struct tbtrace {
 size_t                 tb_entryc;
 struct dtracebackentry tb_entryv[1024];
};

extern __noinline int
tbcapture_walkcount(void const *__restrict instruction_pointer,
                    void const *__restrict frame_address,
                    size_t frame_index, size_t *closure);

extern __noinline int
tbcapture_walkcollect(void const *__restrict instruction_pointer,
                      void const *__restrict frame_address,
                      size_t frame_index, struct tbtrace *tb);

extern int
tbprint_callback(char const *data,
                 size_t max_data,
                 void *closure);
__DECL_END

#endif /* !__TRACEBACK_INTERNAL_H__ */
