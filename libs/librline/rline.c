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
#ifndef __RLINE_C__
#define __RLINE_C__ 1

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <lib/rline.h>
#include <lib/term.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

__DECL_BEGIN

#define HIST_PRESERVE_CURSOR_POSITION 0

static int stub_enumoptions(struct rline *__unused(self),
                            int (*callback)(struct rline *self,
                                            char const *completion)
                                            __attribute_unused,
                            char *__unused(hint)) {
 (void)callback;
 return 0;
}


__public struct rline *
rline_new(struct rline *buf, struct rline_operations const *ops,
          void *closure, int infd, int outfd) {
 struct rline *result;
 if (buf) {
  result = buf;
  result->l_flags = RLINE_FLAG_NONE;
 } else {
  result = omalloc(struct rline);
  if __unlikely(!result) return NULL;
  result->l_flags = RLINE_FLAG_FREE;
 }
 result->l_buffer = (char *)calloc(1,sizeof(char));
 if __unlikely(!result->l_buffer) {
  if (!buf) free(result);
  return NULL;
 }
 result->l_arg    = closure;
 result->l_bufend = result->l_buffer;
 result->l_bufpos = result->l_buffer;
 result->l_bufuse = result->l_buffer;
 result->l_bufold = NULL;
 result->l_infd   = infd;
 result->l_outfd  = outfd;
 result->l_histi  = 0;
 result->l_hista  = 0;
 result->l_histc  = 0;
 result->l_histv  = NULL;
 if (ops) memcpy(&result->l_ops,&ops,sizeof(struct rline_operations));
 else memset(&result->l_ops,0,sizeof(struct rline_operations));
 if (!result->l_ops.lp_enumoptions) result->l_ops.lp_enumoptions = &stub_enumoptions;
 return result;
}
__public int rline_delete(struct rline *self) {
 char **iter,**end;
 if __unlikely(!self) { __set_errno(EINVAL); return -1; }
 end = (iter = self->l_histv)+self->l_histc;
 for (; iter != end; ++iter) free(*iter);
 free(self->l_histv);
 if (self->l_bufold) free(self->l_bufold);
 else free(self->l_buffer);
 if (self->l_flags&RLINE_FLAG_FREE) free(self);
 return 0;
}

#define SELF               self
#define WRITE(buf,bufsize) write(SELF->l_outfd,buf,bufsize)
#define READ(buf,bufsize)  read(SELF->l_infd,buf,bufsize)
#define PRINT(x)           WRITE(x,sizeof(x)-sizeof(char))
#define PRINTF(...)        dprintf(SELF->l_outfd,__VA_ARGS__)

static void rline_refresh(struct rline *self) {
 PRINT("\033[u"); // Restore cursor position
 WRITE(self->l_buffer,(size_t)(self->l_bufuse-self->l_buffer)*sizeof(char));
 PRINT("\033[K"   // Clear the line after the cursor
       "\033[u"); // Restore cursor position
 PRINTF("\033[u" // Restore cursor position
        "\033[%IuC" // Move the cursor to its rightful position
       ,self->l_bufpos-self->l_buffer);
}

static void rline_restoreold(struct rline *self) {
 assert(self->l_bufold);
 self->l_bufpos = self->l_bufold+(self->l_bufpos-self->l_buffer);
 self->l_buffer = self->l_bufold;
 self->l_bufend = strend(self->l_buffer);
 self->l_bufuse = self->l_bufend;
 if (self->l_bufpos > self->l_bufuse) self->l_bufpos = self->l_bufuse;
 self->l_bufold = NULL;
 self->l_histi = self->l_histc;
}

static void rline_histload(struct rline *self) {
 char *histcopy;
 assert((self->l_bufold == NULL) == (self->l_histi == self->l_histc));
 if (!self->l_bufold) return;
 assert(self->l_histv[self->l_histi] == self->l_buffer);
 self->l_histi = self->l_histc;
 histcopy = strdup(self->l_buffer);
 if (!histcopy) {
  // Fallback: Put the rline into a valid state
  rline_restoreold(self);
  rline_refresh(self);
  return;
 }
 free(self->l_bufold);
 self->l_histi = self->l_histc;
 self->l_bufold = NULL;
 self->l_bufpos = histcopy+(self->l_bufpos-self->l_buffer);
 self->l_bufuse = histcopy+(self->l_bufuse-self->l_buffer);
 self->l_buffer = histcopy;
 self->l_bufend = strend(histcopy);
 if (self->l_bufuse > self->l_bufend) self->l_bufuse = self->l_bufend;
 if (self->l_bufpos > self->l_bufuse) self->l_bufpos = self->l_bufuse;
}
static void rline_histsel(struct rline *self, size_t i) {
 char *bufentry;
 assert((self->l_bufold == NULL) == (self->l_histi == self->l_histc));
 if (i >= self->l_histc) i = self->l_histc;
 if (i == self->l_histi) return;
 if (i == self->l_histc) {
  rline_restoreold(self);
  goto refresh;
 } else if (self->l_histi == self->l_histc) {
  assert(!self->l_bufold);
  self->l_bufold = self->l_buffer;
 }
 bufentry = self->l_histv[i];
 self->l_histi = i;
#if HIST_PRESERVE_CURSOR_POSITION
 self->l_bufpos = bufentry+(self->l_bufpos-self->l_buffer);
#endif
 self->l_buffer = bufentry;
 self->l_bufend = strend(bufentry);
 self->l_bufuse = self->l_bufend;
#if HIST_PRESERVE_CURSOR_POSITION
 if (self->l_bufpos > self->l_bufuse)
#endif
 {
  self->l_bufpos = self->l_bufuse;
 }
refresh:
 rline_refresh(self);
}

static void delch(struct rline *self, int before) {
 size_t shift_size;
 rline_histload(self);
 if (before) {
  if (self->l_bufpos == self->l_buffer) return;
  // Navigate ontop of the character
  --self->l_bufpos;
  PRINT("\033[D");
 } else {
  if (self->l_bufpos == self->l_bufuse) return;
 }
 --self->l_bufuse;
 shift_size = (size_t)(self->l_bufuse-self->l_bufpos)*sizeof(char);
 memmove(self->l_bufpos,self->l_bufpos+1,shift_size);
 *self->l_bufuse = '\0';
 // Re-write everything that followed the character
 WRITE(self->l_bufpos,shift_size);
 PRINTF(" \033[%IuD",shift_size+1);
}
static int insch(struct rline *self, char ch, int override) {
 rline_histload(self);
 if (self->l_bufuse == self->l_bufend) {
  size_t newsize; char *newbuffer;
  newsize = (size_t)(self->l_bufend-self->l_buffer);
  newsize = newsize ? newsize*2 : 2;
  newbuffer = (char *)realloc(self->l_buffer,(newsize+1)*sizeof(char));
  if __unlikely(!newbuffer) return -1;
  self->l_bufpos = newbuffer+(self->l_bufpos-self->l_buffer);
  self->l_bufuse = newbuffer+(self->l_bufuse-self->l_buffer);
  self->l_buffer = newbuffer;
  self->l_bufend = newbuffer+newsize;
 }
 if (override || self->l_bufpos == self->l_bufuse) {
  *self->l_bufpos++ = ch;
  WRITE(&ch,sizeof(char));
 } else {
  size_t update_characters;
  memmove(self->l_bufpos+1,self->l_bufpos,
         (self->l_bufuse-self->l_bufpos)*sizeof(char));
  *self->l_bufpos = ch;
  // Re-write the text
  update_characters = ((self->l_bufuse-self->l_bufpos)+1);
  assert(update_characters >= 2);
  WRITE(self->l_bufpos,update_characters*sizeof(char));
  ++self->l_bufpos;
  // Return the cursor to the insert position
  PRINTF("\033[%dD",update_characters-1);
 }
 *++self->l_bufuse = '\0';
 return 0;
}

static void rline_lcur(struct rline *self) {
 if (self->l_bufpos != self->l_buffer) { --self->l_bufpos; PRINT("\033[D"); }
}
static void rline_rcur(struct rline *self) {
 if (self->l_bufpos != self->l_bufuse) { ++self->l_bufpos; PRINT("\033[C"); }
}
static void rline_lcur_n(struct rline *self, unsigned int n) {
 char *newpos;
 if (n == 1) { rline_lcur(self); return; }
 newpos = self->l_bufpos-n;
 if (newpos < self->l_buffer) newpos = self->l_buffer;
 if (newpos != self->l_bufpos) {
  PRINTF("\033[%uD",self->l_bufpos-newpos);
  self->l_bufpos = newpos;
 }
}
static void rline_rcur_n(struct rline *self, unsigned int n) {
 char *newpos;
 if (n == 1) { rline_rcur(self); return; }
 newpos = self->l_bufpos+n;
 if (newpos > self->l_bufuse) newpos = self->l_bufuse;
 if (newpos != self->l_bufpos) {
  PRINTF("\033[%uC",newpos-self->l_bufpos);
  self->l_bufpos = newpos;
 }
}
static void rline_setcur(struct rline *self, char *pos) {
 assert(pos >= self->l_buffer && pos <= self->l_bufend);
 if (pos > self->l_bufuse) pos = self->l_bufuse;
 if (pos < self->l_bufpos) rline_lcur_n(self,self->l_bufpos-pos);
 else rline_rcur_n(self,pos-self->l_bufpos);
}
static void rline_lctrl(struct rline *self) {
 char *end = self->l_buffer;
 char *newcur = self->l_bufpos;
 while (newcur != end &&  isspace(*newcur)) --newcur;
 while (newcur != end && !isspace(*newcur)) --newcur;
 rline_setcur(self,newcur);
}
static void rline_rctrl(struct rline *self) {
 char *end = self->l_bufuse;
 char *newcur = self->l_bufpos;
 while (newcur != end && !isspace(*newcur)) ++newcur;
 while (newcur != end &&  isspace(*newcur)) ++newcur;
 rline_setcur(self,newcur);
}

static void rline_prepare(struct rline *self) {
 char *histentry;
 assert((self->l_bufold == NULL) == (self->l_histi == self->l_histc));
 if (self->l_bufold) { rline_restoreold(self); goto end2; }
 if (self->l_buffer == self->l_bufuse ||
    (self->l_flags&RLINE_FLAG_NOHIST)) goto end;
 if (self->l_histc == self->l_hista) {
  char **newhist;
  size_t newsize = self->l_hista ? self->l_hista*2 : 2;
  newhist = (char **)realloc(self->l_histv,newsize*sizeof(char *));
  if (!newhist) goto end;
  self->l_histv = newhist;
  self->l_hista = newsize;
 }
 histentry = strndup(self->l_buffer,(size_t)(self->l_bufuse-self->l_buffer));
 if __unlikely(!histentry) goto end;
 self->l_histv[self->l_histc++] = histentry;
end:
 self->l_histi = self->l_histc;
end2:
 *self->l_buffer = '\0';
 self->l_bufpos = self->l_buffer;
 self->l_bufuse = self->l_buffer;
}


#define KEY_CTRL(x)   ((x)-'@') /* Shift by 0x40 / 64. */
#define KEY_ISCTRL(x) ((unsigned char)(x) < 32)

__public int rline_read(struct rline *self, char const *prompt) {
 int error; char ch; ssize_t rsize;
 if __unlikely(!self) { __set_errno(EINVAL); return -1; }
 assert(self->l_buffer);
 assert(self->l_bufend);
 assert(self->l_bufpos);
 assert(self->l_bufuse);
 rline_prepare(self);
 if (prompt && *prompt) write(self->l_outfd,prompt,strlen(prompt));
 PRINT("\033[s"); // Save cursor position
 assert((self->l_bufold == NULL) == (self->l_histi == self->l_histc));
 for (;;) {
#define READ_ONE()\
 { if ((rsize = read(self->l_infd,&ch,sizeof(ch))) < (ssize_t)sizeof(ch)) goto read_error; }
  READ_ONE();
  assertf(rsize == sizeof(ch),"%Id",rsize);
  switch (ch) {
   case '\b': delch(self,1); break;
   case '\033':
    /* Parse escape string (arrow keys & such...) */
    READ_ONE();
    if (ch == '[') {
     READ_ONE();
          if (ch == 'D') rline_lcur(self);
     else if (ch == 'C') rline_rcur(self);
     else if (ch == 'A') rline_histsel(self,self->l_histi ? self->l_histi-1 : 0);
     else if (ch == 'B') rline_histsel(self,self->l_histi+1);
     else if (ch == '3') {
      READ_ONE();
      if (ch == '~') delch(self,0);
      else goto skip_escape;
     } else goto skip_escape;
    } else if (ch == 'O') {
     READ_ONE();
          if (ch == 'D') rline_lctrl(self);
     else if (ch == 'C') rline_rctrl(self);
     else if (ch == 'A') rline_histsel(self,self->l_histi ? self->l_histi-1 : 0);
     else if (ch == 'B') rline_histsel(self,self->l_histi+1);
     else if (ch == 'H') rline_setcur(self,self->l_buffer); // HOME
     else if (ch == 'F') rline_setcur(self,self->l_bufuse); // END
     else goto skip_escape;
    } else {
skip_escape:
     while (ch < ANSI_LOW || ch > ANSI_HIGH) READ_ONE();
    }
    break;
   case '\t': break;
   case '\n': PRINT("\n"); goto endok;
   case KEY_CTRL('C'):
    PRINT("^C\n");
    self->l_bufpos = self->l_buffer;
    *self->l_bufpos = '\0';
    goto endok;
    break;
   case KEY_CTRL('D'):
    PRINT("^D");
    error = RLINE_EOF;
    goto end;
   default:
    if (KEY_ISCTRL(ch)) {
#if 1
     // Special escape representation for control characters
     PRINTF("^%c\033[2D",ch+'@');
#endif
    } else {
     if (insch(self,ch,0)) goto err;
    }
    break;
  }
 }
endok: error = RLINE_OK;
end:
 return error;
err: error = RLINE_ERR; goto end;
read_error:
 error = rsize < 0 ? RLINE_ERR : RLINE_EOF;
 goto end;
}


__public char *readline(char const *prompt) {
 return freadline(prompt,STDIN_FILENO,STDOUT_FILENO);
}
__public char *freadline(char const *prompt, int infd, int outfd) {
 struct rline r; char *result = NULL; int error;
 struct termios oldios,newios;
 if (!rline_new(&r,NULL,NULL,infd,outfd)) return NULL;
 r.l_flags |= RLINE_FLAG_NOHIST;
 if (tcgetattr(infd,&oldios) == -1) {
  // Probably just ain't a terminal...
  error = rline_read(&r,prompt);
 } else {
  memcpy(&newios,&oldios,sizeof(struct termios));
  // Disable canon and echo
  newios.c_lflag &= ~(ICANON|ECHO);
  if (tcsetattr(infd,TCSAFLUSH,&newios) == -1) goto end;
  error = rline_read(&r,prompt);
  tcsetattr(infd,TCSAFLUSH,&oldios);
 }
 if (error == RLINE_ERR) result = NULL;
 else if (r.l_bufold) {
  char *send; result = strdup(r.l_buffer);
  if (result && (send = strend(result)) != result &&
      send[-1] == '\n') send[-1] = '\0';
 } else {
  assert(!*r.l_bufpos);
  if (r.l_bufpos == r.l_bufend &&
     (r.l_bufpos == r.l_buffer ||
      r.l_bufpos[-1] != '\n')) result = r.l_buffer;
  else {
   size_t linesize = r.l_bufpos-r.l_buffer;
   if (linesize && r.l_buffer[linesize-1] == '\n') --linesize;
   result = (char *)realloc(r.l_buffer,(linesize+1)*sizeof(char));
   if __unlikely(!result) result = r.l_buffer;
   result[linesize] = '\0';
  }
  r.l_buffer = NULL;
 }
end:
 rline_delete(&r);
 return result;
}


__DECL_END

#endif /* !__RLINE_C__ */
