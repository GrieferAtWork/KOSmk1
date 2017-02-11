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
#ifndef __STDBOOL_H__
#ifndef _STDBOOL_H
#ifndef _INC_STDBOOL
#define __STDBOOL_H__ 1
#define _STDBOOL_H 1
#define _INC_STDBOOL 1

#ifndef __cplusplus
#include <kos/compiler.h>

#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined 1
#ifndef __ASSEMBLY__
__DECL_BEGIN typedef _Bool bool; __DECL_END
#endif /* !__ASSEMBLY__ */
#define true  1
#define false 0
#endif

#endif

#endif /* !_INC_STDBOOL */
#endif /* !_STDBOOL_H */
#endif /* !__STDBOOL_H__ */
