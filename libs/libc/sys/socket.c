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
#ifndef __SYS_SOCKET_C__
#define __SYS_SOCKET_C__ 1

#include <sys/socket.h>
#include <kos/compiler.h>

__DECL_BEGIN

#ifndef __KERNEL__
#undef htons
#undef ntohs
#undef htonl
#undef ntohl
__public unsigned short htons(unsigned short x) { return (unsigned short)__PP_CAT_2(_hton,__PP_MUL8(__SIZEOF_SHORT__))(x); }
__public unsigned short ntohs(unsigned short x) { return (unsigned short)__PP_CAT_2(_ntoh,__PP_MUL8(__SIZEOF_SHORT__))(x); }
__public unsigned long htonl(unsigned long x) { return (unsigned long)__PP_CAT_2(_hton,__PP_MUL8(__SIZEOF_LONG__))(x); }
__public unsigned long ntohl(unsigned long x) { return (unsigned long)__PP_CAT_2(_ntoh,__PP_MUL8(__SIZEOF_LONG__))(x); }
#endif

__DECL_END

#endif /* !__SYS_SOCKET_C__ */
