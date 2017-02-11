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
#ifndef __CURSES_C__
#define __CURSES_C__ 1

#include <curses.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <format-printer.h>
#include <kos/time.h>
#include <kos/task.h>
#include <kos/timespec.h>

#define ANSI_PURE_STD 0

__DECL_BEGIN

#define DEV                (self)
#define print(s)      write(DEV->d_ofd,s,sizeof(s)-sizeof(char))
#define printf(...) dprintf(DEV->d_ofd,__VA_ARGS__)

#define mvcuruone()      print("\033[A")
#define mvcurdone()      print("\033[B")
#define mvcurrone()      print("\033[C")
#define mvcurlone()      print("\033[D")
#define mvcuru(up)       printf("\033[%dA",up)
#define mvcurd(down)     printf("\033[%dB",down)
#define mvcurr(right)    printf("\033[%dC",right)
#define mvcurl(left)     printf("\033[%dD",left)

#define setcurx(x)       printf("\033[%uG",(x)+1u)
#define setcur(x,y)      printf("\033[%u;%uH",(x)+1u,(y)+1u)

__local void d_doprint(struct screen *self,
                       chtype const *iter,
                       chtype const *end) {
#define BUFSIZE 16
 chtype oldattr,newattr,ch;
 char textbuf[BUFSIZE];
 char *buf = textbuf,*bufend = buf+BUFSIZE;
 assert(iter != end);
 oldattr = self->d_attr;
 assert(!(oldattr&~(A_ATTRIBUTES)));
 do {
  ch = *iter;
  newattr = ch&A_ATTRIBUTES;
  if (newattr != oldattr) {
   if (buf != textbuf) {
    write(self->d_ofd,textbuf,(size_t)(buf-textbuf));
    buf = textbuf;
   }
   // TODO: Change terminal attributes (emit escape codes and stuff...)
   if ((oldattr&A_ALTCHARSET) != (newattr&A_ALTCHARSET)) {
    if (newattr&A_ALTCHARSET) {
     print("\033(0"); /* Enable box mode. */
    } else {
     print("\033(B"); /* Disable box mode. */
    }
   }
   //if ((oldattr&A_COLOR) != (newattr&A_COLOR)) {
   // (newattr&A_COLOR) >> 8;
   //}

   oldattr = newattr;
  }
  // Cache the character
  *buf++ = (char)(ch&A_CHARTEXT);
  if (buf == bufend) {
   write(self->d_ofd,textbuf,BUFSIZE);
   buf = textbuf;
  }
 } while (++iter != end);
 if (buf != textbuf) {
  /* Write remaining buffered text. */
  write(self->d_ofd,textbuf,(size_t)(buf-textbuf));
 }
 self->d_attr = newattr;
}

__public int __sdoupdate(struct screen *self) {
 struct devline *iter,*end;
 chtype *curoff = NULL; WINDOW *topwin;
 assert(self);
 end = (iter = self->d_scr.dt_lines)+self->d_size.y;
 for (; iter != end; ++iter) {
  if (iter->dl_chbegin != iter->dl_chend) {
   chtype *liter,*lend;
   liter = iter->dl_chbegin;
   lend = iter->dl_chend;
   assert(liter < lend);
   if (curoff != liter) {
    /* Set the correct cursor position. */
    size_t textoff = (size_t)(curoff-self->d_scr.dt_textbuf);
    setcur(textoff % self->d_size.x,
           textoff / self->d_size.x);
   }
   d_doprint(self,liter,lend);
   curoff = lend;
   iter->dl_chbegin = lend;
  }
 }
 topwin = self->d_top;
 if (!(topwin->w_flags&W_LEAVEOK)) {
  size_t curx,cury;
  assert(topwin->w_curpos >= topwin->w_text.wt_textbuf);
  assert(topwin->w_curpos <= topwin->w_text.wt_textend);
  curx = getcurx(topwin),cury = getcury(topwin);
  if (curoff) {
   size_t textoff = (size_t)(curoff-self->d_scr.dt_textbuf);
   if (curx == textoff % self->d_size.x &&
       cury == textoff / self->d_size.x
       ) goto end; /* Don't need to move the cursor! */
  }
  /* Move the hardware cursor to where this window's cursor is. */
  setcur(curx,cury);
 }
end:
 return OK;
}

/* Returns a pointer to the next character (that is the obstructed character)
   This is used to determine the range of memory that can directly be
   memcpy'ed into the associated device's screen buffer.
   Searching will stop if 'end' is reached, which will then be returned. */
__local chtype *_wnexthidden(WINDOW *win, chtype *start, chtype *end, int find_visible) {
 uint8_t *bytepos,mask;
 size_t start_offset;
 assert(start <= end);
 assert(end <= win->w_text.wt_textend);
 start_offset = (size_t)(start-win->w_text.wt_textbuf);
 bytepos = win->w_text.wt_hidden+(start_offset/8);
 mask = 1 << (start_offset%8);
 while (start != end) {
  /* TODO: By aliging to byte-borders, we
           could check 8 characters at once... */
  if ((*bytepos&mask) ^ find_visible) {
   /* this character is obstructed */
   return start;
  }
  if (mask&0x80) ++bytepos,mask = 0x01;
  else mask <<= 1;
  ++start;
 }
 return find_visible ? NULL : end;
}

__public int wnoutrefresh(WINDOW *win) {
 struct winline *iter,*end;
 chtype *screen_buf,*window_line_begin,*window_line_end;
 size_t screen_scanline,window_scanline;
 struct point dsize; int full_redraw = 0;
 if __unlikely(!win) return ERR;
 /* Sanity assertions. */
 assert(win->w_dim.x == win->w_end.x-win->w_begin.x);
 assert(win->w_dim.y == win->w_end.y-win->w_begin.y);
 assert(win->w_dim.x && win->w_dim.y);
 dsize = win->w_dev->d_size;
 window_scanline = win->w_dim.x;
 end = (iter = win->w_text.wt_lines)+win->w_dim.y;
 screen_buf = win->w_dev->d_scr.dt_textbuf;
 screen_scanline = dsize.x;
 if (win->w_begin.y < 0) {
  /* Clamp out-of-bounds on the top. */
  iter += -win->w_begin.y;
  if (iter >= end) return OK; /* Window is completely out-of-bounds to the top. */
 }
 if (win->w_end.y > dsize.y) {
  /* Clamp out-of-bounds on the bottom. */
  end -= (win->w_end.y-dsize.y);
  if (end <= iter) return OK; /* Window is completely out-of-bounds to the bottom. */
 }
 assert(iter <= end);
 window_line_begin = win->w_text.wt_textbuf+(iter-win->w_text.wt_lines);
 window_line_end = window_line_begin+window_scanline;
 if (win->w_begin.x < 0) {
  /* Clamp out-of-bounds on the left. */
  window_line_begin += -win->w_begin.x;
  if (window_line_begin >= window_line_end)
   return OK; /* Window is completely out-of-bounds to the left. */
 }
 if (win->w_end.x > dsize.x) {
  /* Clamp out-of-bounds on the right. */
  window_line_end -= -(win->w_end.x-dsize.x);
  if (window_line_end <= window_line_begin)
   return OK; /* Window is completely out-of-bounds to the right. */
 }
 if (win->w_flags&W_CLROK) {
  /* Redraw the window from scratch. */
  win->w_flags &= ~(W_CLROK);
  full_redraw = 1;
 }

 for (; iter != end; ++iter,
      window_line_begin += window_scanline,
      window_line_end += window_scanline) {
  if (iter->wl_chbegin != iter->wl_chend) {
   chtype *liter,*lend,*copyend;
   if (full_redraw) {
    liter = window_line_begin;
    lend = window_line_end;
   } else {
    liter = iter->wl_chbegin;
    lend = iter->wl_chend;
    /* Advance 'liter' to exclude out-of-bounds on the left. */
    if (liter < window_line_begin) {
     /* Trim out-of-bounds to the left. */
     liter = window_line_begin;
     if (liter >= lend) goto copy_done; /* No changes were actually on-screen. */
    }
    /* Reduce 'lend' to exclude out-of-bounds on the right. */
    if (lend > window_line_end) {
     lend = window_line_end;
     if (lend <= liter) goto copy_done; /* No changes were actually on-screen. */
    }
   }
   assert(liter < lend);
   while (liter != lend) {
    struct devline *dline;
    chtype *screen_dest;
    /* Calculate where exactly we need to copy the data to on-screen. */
    size_t copysize,offset = liter-win->w_text.wt_textbuf;
    coord_t x = (coord_t)((offset % window_scanline)+win->w_begin.x);
    coord_t y = (coord_t)((offset / window_scanline)+win->w_begin.y);
    assert(x < win->w_dev->d_size.x); // Checked above
    assert(y < win->w_dev->d_size.y); // Checked above
    /* Figure out how much we can copy at once (Considering obstructions) */
    copyend = _wnexthidden(win,liter,lend,0);
    screen_dest = screen_buf+y*screen_scanline+x;
    copysize = (size_t)(copyend-liter);
    /* Actually perform the copy. */
    memcpy(screen_dest,liter,copysize*sizeof(chtype));
    /* Make our changes relevant on-screen. */
    dline = win->w_dev->d_scr.dt_lines+y;
    if (dline->dl_chbegin == dline->dl_chend) {
     /* First change on this line. */
     dline->dl_chbegin = screen_dest;
     dline->dl_chend   = screen_dest+copysize;
    } else {
     /* Do an inclusive change update. */
     if (screen_dest        < dline->dl_chbegin) dline->dl_chbegin = screen_dest;
     if (screen_dest+copysize > dline->dl_chend) dline->dl_chend = screen_dest+copysize;
    }
    /* Skip all obstructed characters to continue copying the next part afterwards. */
    if (copyend == lend) break;
    liter = _wnexthidden(win,copyend,lend,1);
    /* If there are no more visible chunks left, stop. */
    if (!liter) break;
   }
copy_done:
   iter->wl_chbegin = lend;
  }
 }
 return OK;
}

__public int wrefresh(WINDOW *win) {
 if __unlikely(!win) return ERR;
 if (wnoutrefresh(win) == ERR) return ERR;
 return __sdoupdate(win->w_dev);
}
__public int wredrawln(WINDOW *win, int beg_line, int num_lines) {
 struct winline *iter,*end;
 chtype *liter; size_t scanline;
 if __unlikely(!win) return ERR;
      if (beg_line < 0) beg_line = 0;
 else if (beg_line >= win->w_dim.y) return OK;
 scanline = win->w_dim.x;
 liter = win->w_text.wt_textbuf+(beg_line*scanline);
 iter = win->w_text.wt_lines+beg_line;
 end = iter+min(num_lines,win->w_dim.y-beg_line);
 for (; iter != end; ++iter,liter += scanline) {
  iter->wl_chbegin = liter;
  iter->wl_chend = liter+1;
 }
 return OK;
}
__public int redrawwin(WINDOW *win) {
 struct winline *iter,*end;
 chtype *liter; size_t scanline;
 if __unlikely(!win) return ERR;
 scanline = win->w_dim.x;
 liter = win->w_text.wt_textbuf;
 iter = win->w_text.wt_lines;
 end = iter+win->w_dim.y;
 for (; iter != end; ++iter,liter += scanline) {
  iter->wl_chbegin = liter;
  iter->wl_chend = liter+1;
 }
 return OK;
}

#undef doupdate
__public int doupdate(void) {
 return __sdoupdate(__curterm);
}



__public int wmove(WINDOW *win, int y, int x) {
 if __unlikely(!win) return ERR;
 if (x < 0 || y < 0 ||
     x >= win->w_dim.x ||
     y >= win->w_dim.y) return ERR;
 win->w_curpos = win->w_text.wt_textbuf+(y*win->w_dim.x)+x;
 return OK;
}

__local void win_chgn_all(WINDOW *win) {
 struct winline *liter,*lend; chtype *dst;
 size_t scanline = win->w_dim.x;
 lend = (liter = win->w_text.wt_lines)+win->w_dim.y;
 dst = win->w_text.wt_textbuf;
 for (; liter != lend; ++liter,dst += scanline) {
  liter->wl_chbegin = dst;
  liter->wl_chend = dst+scanline;
 }
}

__public int idlok(WINDOW *win, bool bf) {
#ifndef __INTELLISENSE__
 return __curses_wsetflag(win,bf,__CURSES_WINDOWFLAG_IDLOK);
#endif
}
__public void immedok(WINDOW *win, bool bf) {
#ifndef __INTELLISENSE__
 __curses_wsetflag(win,bf,__CURSES_WINDOWFLAG_IMMEDOK);
#endif
 if (bf) wrefresh(win);
}


__public int wscrl(WINDOW *win, int n) {
 chtype *dst,*src,filler;
 size_t scanline,height;
 scanline = win->w_dim.x;
 height = win->w_dim.y;
 if __unlikely(!win) return ERR;
 if __unlikely(!n) return OK;
 if (n < 0) {
  n = -n;
  if (n >= height) {
clear_win:
   return wclear(win);
  }
  /* Scroll downwards */
  src = win->w_text.wt_textbuf;
  dst = src+n*scanline;
  memmove(dst,src,(height-n)*scanline*sizeof(chtype));
  dst = src;
 } else {
  if (n >= height) goto clear_win;
  /* Scroll upwards */ 
  dst = win->w_text.wt_textbuf;
  src = dst+n*scanline;
  memmove(dst,src,(height-n)*scanline*sizeof(chtype));
  dst = win->w_text.wt_textend-(n*scanline);
 }
 /* Fill unused space with background */
 filler = win->w_bkgd|' ';
 src = dst+n*scanline;
 while (dst != src) *dst++ = filler;
 /* Mark everything as changed. */
 win_chgn_all(win);
 return OK;
}

__public int wclrtobot(WINDOW *win) {
 size_t offset; chtype *iter,*end;
 struct winline *line,*lend;
 if __unlikely(!win) return ERR;
 iter = win->w_curpos;
 offset = (size_t)(win->w_curpos-win->w_text.wt_textbuf);
 line = win->w_text.wt_lines+(offset / win->w_dim.x);
 end = win->w_curpos+(win->w_dim.x-(offset % win->w_dim.x));
 if (line->wl_chbegin == line->wl_chend) {
  line->wl_chbegin = iter;
  line->wl_chend = end;
 } else {
  if (iter < line->wl_chbegin) line->wl_chbegin = iter;
  if (end > line->wl_chend) line->wl_chend = end;
 }
 for (; iter != end; ++iter) *iter = win->w_bkgd|' ';
 lend = win->w_text.wt_lines+win->w_dim.y;
 ++line;
 for (; line != lend; ++line) {
  iter = end;
  end = iter+win->w_dim.x;
  line->wl_chbegin = iter;
  line->wl_chend = end;
  for (; iter != end; ++iter) {
   *iter = win->w_bkgd|' ';
  }
 }
 return OK;
}
__public int wclrtoeol(WINDOW *win) {
 size_t offset; chtype *iter,*end;
 struct winline *line;
 if __unlikely(!win) return ERR;
 iter = win->w_curpos;
 offset = (size_t)(win->w_curpos-win->w_text.wt_textbuf);
 line = win->w_text.wt_lines+(offset / win->w_dim.x);
 end = win->w_curpos+(win->w_dim.x-(offset % win->w_dim.x));
 if (line->wl_chbegin == line->wl_chend) {
  line->wl_chbegin = iter;
  line->wl_chend = end;
 } else {
  if (iter < line->wl_chbegin) line->wl_chbegin = iter;
  if (end > line->wl_chend) line->wl_chend = end;
 }
 for (; iter != end; ++iter) {
  *iter = win->w_bkgd|' ';
 }
 return OK;
}



__public int waddch(WINDOW *win, chtype const ch) {
 unsigned char xch; chtype *cur; size_t offset;
 struct winline *wline;
 if __unlikely(!win) return ERR;
 xch = (unsigned char)(ch&A_CHARTEXT);
 /* Check for control/special character */
 if (xch < 32) switch (xch) {
  case '\n':
   wclrtoeol(win);
   cur = win->w_text.wt_textbuf;
   cur += win->w_dim.x-((cur-win->w_text.wt_textbuf) % win->w_dim.x);
   assert(cur <= win->w_text.wt_textend);
   if (cur == win->w_text.wt_textend) {
    scroll(win);
    cur -= win->w_dim.x;
   }
   goto end;
  default:
   /* Fallback: Write the ^-style escaped representation of 'ch'. */
   if (waddch(win,'^'|(ch&A_ATTRIBUTES)) == ERR) return ERR;
   return waddch(win,(xch+0x40)|(ch&A_ATTRIBUTES));
 }
 if ((cur = win->w_curpos) == win->w_text.wt_textend) {
  if (!(win->w_flags&W_SCRLOK)) {
   /* Scroll one line, if scroll-lock isn't enabled. */
   if (scroll(win) == ERR) return ERR;
  }
  /* Wrap cursor on last line. */
  cur = win->w_curpos = win->w_text.wt_textend-win->w_dim.x;
 }
 offset = (size_t)(cur-win->w_text.wt_textbuf);
 wline = win->w_text.wt_lines+(offset/win->w_dim.x);
 assert(wline >= win->w_text.wt_lines &&
        wline < win->w_text.wt_lines+win->w_dim.y);
 /* Mark this character's line as changed. */
 if (wline->wl_chbegin == wline->wl_chend) {
  wline->wl_chbegin = cur;
  wline->wl_chend   = cur;
 } else {
  if (cur < wline->wl_chbegin) wline->wl_chbegin = cur;
  if (cur > wline->wl_chend) wline->wl_chend = cur;
 }
 *cur++ = win->w_bkgd|ch;
end:
 win->w_curpos = cur;
 return OK;
}

__public int wechochar(WINDOW *win, chtype const ch) {
 if (waddch(win,ch) == ERR) return ERR;
 return wrefresh(win);
}



__public SCREEN *__curterm = NULL;
__public SCREEN  __stdterm;
__public WINDOW  __stdsrc_;

static void restore_screen(SCREEN *self) {
 print("\033[1;4096H"); /* Move cursor to bottom-left corner. */
}

static void quit_screen_info(SCREEN *self) {
 restore_screen(self);
 free(self->d_scr.dt_lines);
 free(self->d_scr.dt_textbuf);
}

static chtype *alloc_text(size_t charcount) {
 chtype *iter,*end;
 chtype *result = (chtype *)(malloc)(charcount*sizeof(chtype));
 if (!result) return NULL;
 end = (iter = result)+charcount;
 for (; iter != end; ++iter) *iter = ' ';
 return result;
}


static int init_screen_info(SCREEN *self) {
 char answer[64]; ssize_t rsize; int sx,sy;
 if (!isatty(self->d_ifd) || !isatty(self->d_ofd)) {
  dprintf(STDERR_FILENO,
          "[CURSES] stdin/stdout are not terminals: %s\n",
          strerror(errno));
  return ERR;
 }
 print("\033[s" // Save cursor position
       "\033[4096;4096H"); /* Move the cursor to some high position. */
 rsize = read(self->d_ifd,answer,sizeof(answer));
 if (rsize < 0) return -1;
 if (sscanf(answer,"\033[%d;%dR",&sx,&sy) != 2 || sx <= 1 || sy <= 1) {
  dprintf(STDERR_FILENO,
          "[CURSES] Unrecognized response to size query: '%s'\n",
          answer);
  return ERR;
 }
 /* We've figured out the terminal size! */
 self->d_size.x = sx;
 self->d_size.y = sy;
 self->d_attr = A_NORMAL;
 self->d_scr.dt_lines = (struct devline *)(calloc)(sy,sizeof(struct devline));
 if __unlikely(!self->d_scr.dt_lines) goto err;
 self->d_scr.dt_textbuf = alloc_text(sx*sy);
 if __unlikely(!self->d_scr.dt_textbuf) goto err_lines;
 self->d_scr.dt_textend = self->d_scr.dt_textbuf+sx*sy;
 return OK;
err_lines: free(self->d_scr.dt_lines);
err:       print("\033[u"); // Restore cursor position
 return ERR;
}

/* Initialize 'w_text' and 'w_curpos' */
static int init_window(WINDOW *self) {
 size_t text_size = self->w_dim.x*self->w_dim.y;
 self->w_text.wt_textbuf = alloc_text(text_size);
 if __unlikely(!self->w_text.wt_textbuf) return ERR;
 self->w_text.wt_textend = self->w_text.wt_textbuf+text_size;
 self->w_text.wt_hidden = (uint8_t *)(calloc)(1,ceildiv(text_size,8));
 if __unlikely(!self->w_text.wt_hidden) goto err_buf;
 self->w_text.wt_lines = (struct winline *)(calloc)(self->w_dim.y,sizeof(struct winline));
 if __unlikely(!self->w_text.wt_lines) goto err_hidden;
 self->w_curpos = self->w_text.wt_textbuf;
 return OK;
err_hidden: free(self->w_text.wt_hidden);
err_buf: free(self->w_text.wt_textbuf);
 return ERR;
}


__public WINDOW *initscr(void) {
 if (!__curterm) {
  __curterm = &__stdterm;
  __stdterm.d_ifd = STDIN_FILENO;
  __stdterm.d_ofd = STDOUT_FILENO;
  __stdterm.d_top = &__stdsrc_;
  if (init_screen_info(&__stdterm) == ERR) {
reset_term:
   memset(&__stdterm,0,sizeof(__stdterm));
   __curterm = NULL;
   return NULL;
  }
  __stdsrc_.w_zprev = NULL;
  __stdsrc_.w_znext = NULL;
  __stdsrc_.w_dev = &__stdterm;
  __stdsrc_.w_parent = NULL;
  __stdsrc_.w_ppos.x = 0;
  __stdsrc_.w_ppos.y = 0;
  __stdsrc_.w_begin.x = 0;
  __stdsrc_.w_begin.y = 0;
  __stdsrc_.w_end.x = __stdterm.d_size.x;
  __stdsrc_.w_end.y = __stdterm.d_size.y;
  __stdsrc_.w_dim.x = __stdterm.d_size.x;
  __stdsrc_.w_dim.y = __stdterm.d_size.y;
  __stdsrc_.w_bkgd = 0;
  __stdsrc_.w_flags = W_IDCOK;
  if (init_window(&__stdsrc_) == ERR) {
   memset(&__stdsrc_,0,sizeof(__stdsrc_));
   quit_screen_info(&__stdterm);
   goto reset_term;
  }
 }
 return stdsrc;
}
__public int endwin(void) {
 if __unlikely(!__curterm) return ERR;
 restore_screen(__curterm);
 return OK;
}
__public bool isendwin(void) { return __curterm == NULL; }

__public SCREEN *newterm(char *type, FILE *outfd, FILE *infd) {
 SCREEN *result = omalloc(SCREEN);
 if __unlikely(!result) return NULL;
 result->d_ifd = fileno(infd);
 result->d_ofd = fileno(outfd);
 result->d_top = NULL;
 if (result->d_ifd == -1 || result->d_ofd == -1) goto err;
 if (init_screen_info(result) == ERR) goto err;
err: free(result);
 return NULL;
}

__public SCREEN *set_term(SCREEN *new_) {
 SCREEN *result = __curterm;
 __curterm = new_;
 return result;
}


__public void delscreen(SCREEN* sp) {
 if __unlikely(!sp) return;
 if (sp == __curterm) {
  __curterm = sp == &__stdterm ? NULL : &__stdterm;
 }
 quit_screen_info(sp);
 if (sp != &__stdterm) free(sp);
}






__public int
wborder(WINDOW *win, chtype ls, chtype rs,
        chtype ts, chtype bs, chtype tl,
        chtype tr, chtype bl, chtype br) {
 if (!ls) ls = ACS_VLINE;
 if (!rs) rs = ACS_VLINE;
 if (!ts) ts = ACS_HLINE;
 if (!bs) bs = ACS_HLINE;
 if (!tl) tl = ACS_ULCORNER;
 if (!tr) tl = ACS_URCORNER;
 if (!bl) tl = ACS_LLCORNER;
 if (!br) tl = ACS_LRCORNER;
 wmove(win,0,0);
 waddch(win,tl);
 if (win->w_dim.x > 2) whline(win,ts,win->w_dim.x-2);
 waddch(win,tr);
 if (win->w_dim.y > 2) wvline(win,ls,win->w_dim.y-2);
 waddch(win,bl);
 if (win->w_dim.x > 2) whline(win,bs,win->w_dim.x-2);
 waddch(win,br);
 if (win->w_dim.y > 2) mvwvline(win,1,win->w_dim.x-1,rs,win->w_dim.y-2);
 return OK;
}
__public int whline(WINDOW *win, chtype ch, int n) {
 while (n--) if (waddch(win,ch) == ERR) return ERR;
 return OK;
}
__public int wvline(WINDOW *win, chtype ch, int n) {
 int y,x;
 if (getyx(win,y,x) == ERR) return ERR;
 while (n--) {
  if (waddch(win,ch) == ERR) return ERR;
  if (wmove(win,++y,x) == ERR) return ERR;
 }
 return OK;
}


#undef mvwhline
__public int mvwhline(WINDOW *win, int y, int x, chtype ch, int n) {
 if (wmove(win,y,x) == ERR) return ERR;
 return whline(win,ch,n);
}
#undef mvwvline
__public int mvwvline(WINDOW *win, int y, int x, chtype ch, int n) {
 if (wmove(win,y,x) == ERR) return ERR;
 return wvline(win,ch,n);
}
#undef hline
__public int hline(chtype ch, int n) { return whline(stdsrc,ch,n); }
#undef vline
__public int vline(chtype ch, int n) { return wvline(stdsrc,ch,n); }
#undef mvhline
__public int mvhline(int y, int x, chtype ch, int n) { return mvwhline(stdsrc,y,x,ch,n); }
#undef mvvline
__public int mvvline(int y, int x, chtype ch, int n) { return mvwvline(stdsrc,y,x,ch,n); }


__public void wbkgdset(WINDOW *win, chtype ch) {
 if (!win) return;
 win->w_bkgd = ch;
}
__public int wbkgd(WINDOW *win, chtype ch) {
 chtype *iter,*end;
 if (!win) return ERR;
 if (win->w_bkgd == ch) return OK;
 win->w_bkgd = ch;
 if (!ch) return OK;
 iter = win->w_text.wt_textbuf;
 end = win->w_text.wt_textend;
 for (; iter != end; ++iter) *iter |= ch;
 win_chgn_all(win);
 return OK;
}

__public chtype getbkgd(WINDOW *win) {
 return win ? win->w_bkgd : (chtype)ERR;
}


__public int waddchnstr(WINDOW *win, chtype const *chstr, int n) {
 chtype *dest,*line_end;
 size_t offset,copy_size; struct winline *line;
 if (!win) return ERR;
 dest = win->w_curpos;
 offset = (size_t)(dest-win->w_text.wt_textbuf);
 line_end = dest+(win->w_dim.x-(offset % win->w_dim.x));
 copy_size = min((size_t)*(unsigned int *)&n,(size_t)(line_end-dest));
 line_end = dest+copy_size;
 memcpy(dest,chstr,copy_size);
 line = win->w_text.wt_lines+(offset / win->w_dim.x);
 if (line->wl_chbegin == line->wl_chend) {
  line->wl_chbegin = dest;
  line->wl_chend = line_end;
 } else {
  if (dest < line->wl_chbegin) line->wl_chbegin = dest;
  if (line_end > line->wl_chend) line->wl_chend = line_end;
 }
 return OK;
}

__public int waddstr(WINDOW *win, const char *str) {
 return waddnstr(win,str,-1);
}
__public int waddnstr(WINDOW *win, const char *str, int n) {
 size_t strsize = strnlen(str,(size_t)*(unsigned int *)&n);
 while (strsize--) if (waddch(win,*str++) == ERR) return ERR;
 return OK;
}


static int vwprintw_callback(char const *str, size_t maxlen, WINDOW *w) {
 return waddnstr(w,str,(int)maxlen); /* OH GOD! This cast hurts SO much! */
}
__public int vwprintw(WINDOW *win, const char *fmt, va_list varglist) {
 return format_vprintf((pformatprinter)&vwprintw_callback,win,fmt,varglist);
}
__public int printw(const char *fmt, ...) {
 va_list args; int result;
 va_start(args,fmt);
 result = vwprintw(stdsrc,fmt,args);
 va_end(args);
 return result;
}
__public int wprintw(WINDOW *win, const char *fmt, ...) {
 va_list args; int result;
 va_start(args,fmt);
 result = vwprintw(win,fmt,args);
 va_end(args);
 return result;
}
__public int mvprintw(int y, int x, const char *fmt, ...) {
 va_list args; int result;
 if (wmove(stdsrc,y,x) == ERR) return ERR;
 va_start(args,fmt);
 result = vwprintw(stdsrc,fmt,args);
 va_end(args);
 return result;
}
__public int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...) {
 va_list args; int result;
 if (wmove(win,y,x) == ERR) return ERR;
 va_start(args,fmt);
 result = vwprintw(win,fmt,args);
 va_end(args);
 return result;
}
__public __compiler_ALIAS(vw_printw,vwprintw);


#undef DEV
#define DEV  __curterm
__public int curs_set(int visibility) {
 if (!DEV) return ERR;
 if (visibility) {
  print("\033[25h");
 } else {
  print("\033[25l");
 }
 return OK;
}

__public int napms(int ms) {
 struct timespec abstime;
 if (KE_ISERR(ktime_getnoworcpu(&abstime))) return ERR;
 abstime.tv_nsec += (ms%1000)*1000000;
 abstime.tv_sec  += (ms/1000);
 while (abstime.tv_nsec >= 1000000000) {
  abstime.tv_nsec -= 1000000000;
  ++abstime.tv_sec;
 }
 if (KE_ISERR(ktask_abssleep(ktask_self(),&abstime))) return ERR;
 return OK;
}

__public int beep(void) {
 /* TODO: Someday, when I've written an audio driver? */
 return flash();
}
__public int flash(void) {
 if (!DEV) return ERR;
 print("\07");
 return OK;
}



__DECL_END

#endif /* !__CURSES_C__ */
