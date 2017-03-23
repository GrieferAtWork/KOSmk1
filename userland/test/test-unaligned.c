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

#include "helper.h"
#include <assert.h>
#include <stddef.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>

static char data[] = "<<Unaligned-text>>";

TEST(unaligned) {
 size_t pagesize;
 ssize_t s;
 char *p,*unaligned;
 char buf[sizeof(data)];
 int pipes[2];

 /* Make sure the kernel is able to handle unaligned user-space pointers. */
 pagesize = sysconf(_SC_PAGESIZE);
 assert(pagesize);

 p = (char *)memalign(pagesize,pagesize*2);
 /* Align the pointer so it wraps across multiple pages. */
 unaligned = p+pagesize-sizeof(data)/2;
 assert(unaligned >= p);

 memcpy(unaligned,data,sizeof(data));
 assertf(!memcmp(data,unaligned,sizeof(data)),"%.?q != %.?q",
         sizeof(data),data,sizeof(data),unaligned);

 ASSERT_ERRNO(pipe(pipes) != -1);

 /* Write some data through the pipe. */
 s = write(pipes[1],unaligned,sizeof(data));
 ASSERT_ERRNO(s != -1);
 assertf(s == sizeof(data),"%Id",s);
 assertf(!memcmp(data,unaligned,sizeof(data)),"%.?q != %.?q",
         sizeof(data),data,sizeof(data),unaligned); /* Sanity check: Write didn't modify the buffer. */

 /* Read the previously written data back. */
 s = read(pipes[0],buf,sizeof(data));
 ASSERT_ERRNO(s != -1);
 assertf(s == sizeof(data),"%Id",s);
 ASSERT_ERRNO(close(pipes[0]) != -1);
 ASSERT_ERRNO(close(pipes[1]) != -1);

 /* Make sure the output matches the input. */
 assertf(!memcmp(buf,data,sizeof(data)),"%.?q != %.?q",
         sizeof(data),buf,sizeof(data),data);

 free(p);
}
