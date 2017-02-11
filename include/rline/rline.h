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
#ifndef __RLINE_RLINE_H__
#define __RLINE_RLINE_H__ 1

#include <kos/compiler.h>
#include <stddef.h>
#include <malloc.h>
#include <stdint.h>

__DECL_BEGIN

struct rline;

struct kbevent; // From <kos/keyboard.h>

typedef int (*penumoptions)(struct rline *self,
                            int (*callback)(struct rline *self,
                                            char const *completion),
                            char *hint);

struct rline_operations {
 penumoptions lp_enumoptions; /*< [0..1] Enumerate possible options, given a 'hint' that is the current word (Used to implement tab-complete). */
};

struct rline {
 void                   *l_arg;    /*< [?..?] User-provided closure argument. */
 char                   *l_buffer; /*< [1..1][owned_if(!l_bufold)] Input buffer. */
 char                   *l_bufpos; /*< [1..1] Current cursor position in the input buffer. */
 char                   *l_bufuse; /*< [1..1] End of the used input buffer. */
 char                   *l_bufend; /*< [1..1] End of the allocated input buffer. */
 char                   *l_bufold; /*< [0..1][owned] Held buffer while navigating within history. */
 int                     l_infd;   /*< File descriptor used for input data (usually STDIN_FILENO). */
 int                     l_outfd;  /*< File descriptor used for output data (usually STDERR_FILENO). */
#define RLINE_FLAG_NONE   0x00000000
#define RLINE_FLAG_FREE   0x00000001 /*< rline_delete should free(self). */
#define RLINE_FLAG_NOHIST 0x00000002 /*< Don't create historical rline entires. */
 uint32_t                l_flags;  /*< RLine flags. */
 struct rline_operations l_ops;    /*< RLine callbacks. */
 size_t                  l_histi;  /*< Current history index (== 'l_histc' when not borwsing history). */
 size_t                  l_hista;  /*< Allocate history size. */
 size_t                  l_histc;  /*< Used history size. */
 char                  **l_histv;  /*< [1..1][owned][0..l_histc|alloc(l_hista)][owned] Historical text vector. */
};

extern struct rline *
rline_new(struct rline *buf, struct rline_operations const *ops,
          void *closure, int infd, int outfd);
extern int rline_delete(struct rline *self);

//////////////////////////////////////////////////////////////////////////
// Fill/Overwrite the contents of a given rline with possibly interactive data.
// An existing non-empty buffer is added to the rline backlog.
// NOTE: If the set input/output descriptors are terminals,
//       the caller should disable CANON and ECHO.
extern int rline_read(struct rline *self, char const *prompt);
#define RLINE_OK    0
#define RLINE_ERR (-1)
#define RLINE_EOF   1

//////////////////////////////////////////////////////////////////////////
// Returns the text and its size after a call to
// 'rline_read', that did not return 'RLINE_ERR'.
// NOTE: 'rline_text' returns a zero-terminated text,
//        with the caller being allowed to assume:
//     >> assert(strlen(rline_text(r)) == rline_size(r));
// NOTE: 'rline_text' never returns NULL.
// NOTE: The text returned by 'rline_text' may end with a '\n' character.
#define rline_text(self) ((self)->l_buffer)
#define rline_size(self) ((size_t)((self)->l_bufuse-(self)->l_buffer))


// Standard readline support
extern char *readline(char const *prompt);
extern char *freadline(char const *prompt, int infd, int outfd);

__DECL_END

#endif /* !__RLINE_RLINE_H__ */
