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
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <format-printer.h>
#include <kos/time.h>
#include <kos/task.h>
#include <kos/timespec.h>
#include <alloca.h>
#include <ctype.h>

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


#define BUFAT(win,y_,x_) \
 ((win)->w_text.wt_textbuf+(x_)+(y_)*(win)->w_dim.x)



__local void d_doprint(SCREEN *self,
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

static int __sdoupdate(SCREEN *self, WINDOW *topwin) {
 struct devline *iter,*end;
 chtype *curoff = NULL;
 assert(self);
 assert(topwin);
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
 return __sdoupdate(win->w_dev,win);
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
__public int doupdate(void) {
 return __sdoupdate(__curterm,stdsrc);
}



__public int wmove(WINDOW *win, int y, int x) {
 if __unlikely(!win) return ERR;
 if (x < 0 || y < 0 ||
     x >= win->w_dim.x ||
     y >= win->w_dim.y) return ERR;
 win->w_curpos = win->w_text.wt_textbuf+(y*win->w_dim.x)+x;
 return OK;
}

__local void win_changedall(WINDOW *win) {
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
 win_changedall(win);
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

static int screen_saveios(SCREEN *self) {
 return tcgetattr(self->d_ifd,&self->d_oios);
}
static int screen_restoreios(SCREEN *self) {
 return tcsetattr(self->d_ifd,TCSAFLUSH,&self->d_oios);
}

__public SCREEN *__curterm = NULL;
__public SCREEN  __stdterm;
__public WINDOW  __stdsrc_;

static void restore_screen(SCREEN *self) {
 print("\033[1;4096H"); /* Move cursor to bottom-left corner. */
 screen_restoreios(self);
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


static int getenv_screen_size(int *x, int *y) {
 char const *a,*b;
 if ((a = getenv("LINES")) != NULL &&
     (b = getenv("COLS")) != NULL) {
  *x = atoi(b);
  *y = atoi(a);
  return OK;
 }
 if ((a = getenv("TERMSIZE")) != NULL &&
     sscanf(a,"%d;%d",x,y) == 2) return OK;

 return ERR;
}


static int init_screen_info(SCREEN *self) {
 char answer[64]; ssize_t rsize; int sx,sy;
 if (!isatty(self->d_ifd) || !isatty(self->d_ofd)) {
  dprintf(STDERR_FILENO,
          "[CURSES] stdin/stdout are not terminals: %s\n",
          strerror(errno));
  return ERR;
 }
 print("\033[s"          /* Save cursor position */
       "\033[4096;4096H" /* Move the cursor to some high position. */
       "\033[n"          /* Query cursor position. */
       );
 rsize = read(self->d_ifd,answer,sizeof(answer));
 if (rsize < 0) return -1;
 if (rsize == 0) {
use_env_screen_size:
  if (getenv_screen_size(&sx,&sy) == ERR) return ERR;
  goto has_screen_size;
 }
 if (sscanf(answer,"\033[%d;%dR",&sx,&sy) != 2 || sx <= 1 || sy <= 1) {
  dprintf(STDERR_FILENO,
          "[CURSES] Unrecognized response to size query: %q\n",
          answer);
  goto use_env_screen_size;
 }
 {
  int esx,esy;
  /* All environment variables to lower the used screen size. */
  if (getenv_screen_size(&esx,&esy) == OK) {
   if (esx < sx) sx = esx;
   if (esy < sy) sy = esy;
  }
 }
has_screen_size:

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


__public int __curses_setlflag(tcflag_t flag, int enabled) {
 tcflag_t oldflags,newflags;
 if (!__curterm) return ERR;
 newflags = oldflags = __curterm->d_cios.c_lflag;
 if (enabled) newflags |=  (flag);
 else         newflags &= ~(flag);
 if (newflags != oldflags) {
  __curterm->d_cios.c_lflag = newflags;
  if (tcsetattr(__curterm->d_ifd,TCSAFLUSH,&__curterm->d_cios) != 0) {
   __curterm->d_cios.c_lflag = oldflags;
   return ERR;
  }
 }
 return OK;
}
__public int __curses_setecho(int enabled) {
 tcflag_t oldflags,newflags;
 if (!__curterm) return ERR;
 newflags = oldflags = __curterm->d_cios.c_lflag;
 if (enabled) newflags |=  (ECHO);
 else         newflags &= ~(ECHO);
 if (newflags != oldflags) {
  __curterm->d_cios.c_lflag = newflags;
  if (tcsetattr(__curterm->d_ifd,TCSAFLUSH,&__curterm->d_cios) != 0) {
   __curterm->d_cios.c_lflag = oldflags;
   return ERR;
  }
 }
 return OK;
}

__public WINDOW *initscr(void) {
 if (!__curterm) {
  __stdterm.d_ifd = STDIN_FILENO;
  __stdterm.d_ofd = STDOUT_FILENO;
  if (screen_saveios(&__stdterm) != 0) return NULL;
  __stdterm.d_cios = __stdterm.d_oios;
  __curterm = &__stdterm;
  __stdterm.d_top = &__stdsrc_;
  if (init_screen_info(&__stdterm) == ERR) {
reset_term:
   memset(&__stdterm,0,sizeof(__stdterm));
   __curterm = NULL;
   return NULL;
  }
  __stdsrc_.w_dev     = &__stdterm;
  __stdsrc_.w_parent  = NULL;
  __stdsrc_.w_childc  = 0;
  __stdsrc_.w_childv  = NULL;
  __stdsrc_.w_ppos.x  = 0;
  __stdsrc_.w_ppos.y  = 0;
  __stdsrc_.w_begin.x = 0;
  __stdsrc_.w_begin.y = 0;
  __stdsrc_.w_end.x   = __stdterm.d_size.x;
  __stdsrc_.w_end.y   = __stdterm.d_size.y;
  __stdsrc_.w_dim.x   = __stdterm.d_size.x;
  __stdsrc_.w_dim.y   = __stdterm.d_size.y;
  __stdsrc_.w_bkgd    = 0;
  __stdsrc_.w_flags   = W_DEFAULT;
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
 __curterm = NULL;
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
 win_changedall(win);
 return OK;
}

__public chtype getbkgd(WINDOW *win) {
 return win ? win->w_bkgd : (chtype)ERR;
}


__local void __waddchnstrat(WINDOW *win, chtype *dest, chtype const *chstr, int n) {
 chtype *line_end;
 size_t offset,copy_size; struct winline *line;
 assertf(dest >= win->w_text.wt_textbuf,"Dest begin %p is out of bounds: %p < %p",dest,win->w_text.wt_textbuf);
 assertf(dest+n <= win->w_text.wt_textend,"Dest end %p is out of bounds: %p > %p",dest+n,win->w_text.wt_textend);
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
}
__local void __waddchnstrat_overlay(WINDOW *win, chtype *dest,
                                    chtype const *chstr, int n) {
 chtype *line_end,*iter,*end;
 size_t offset,copy_size; struct winline *line;
 assertf(dest >= win->w_text.wt_textbuf,"Dest begin %p is out of bounds: %p < %p",dest,win->w_text.wt_textbuf);
 assertf(dest+n <= win->w_text.wt_textend,"Dest end %p is out of bounds: %p > %p",dest+n,win->w_text.wt_textend);
 offset = (size_t)(dest-win->w_text.wt_textbuf);
 line_end = dest+(win->w_dim.x-(offset % win->w_dim.x));
 copy_size = min((size_t)*(unsigned int *)&n,(size_t)(line_end-dest));
 line_end = dest+copy_size;
 end = (iter = dest)+copy_size;
 for (; iter != end; ++iter,++chstr) if (!isspace(*chstr&A_CHARTEXT)) *iter = *chstr;
 line = win->w_text.wt_lines+(offset / win->w_dim.x);
 if (line->wl_chbegin == line->wl_chend) {
  line->wl_chbegin = dest;
  line->wl_chend = line_end;
 } else {
  if (dest < line->wl_chbegin) line->wl_chbegin = dest;
  if (line_end > line->wl_chend) line->wl_chend = line_end;
 }
}


__public int waddchnstr(WINDOW *win, chtype const *chstr, int n) {
 if __unlikely(!win) return ERR;
 __waddchnstrat(win,win->w_curpos,chstr,n);
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
__public __COMPILER_ALIAS(vw_printw,vwprintw);


__public int
copywin(WINDOW const *srcwin, WINDOW *dstwin, int sminrow, int smincol,
        int dminrow, int dmincol, int dmaxrow, int dmaxcol, int overlay) {
 unsigned int sizex,sizey; chtype *dst,*src;
 unsigned int fill_x_before,copy_x,fill_x_after;
 unsigned int fill_y_before,copy_y,fill_y_after;
 unsigned int scan_src,scan_dst;
 chtype *filler = NULL,*iter,*end;
 if (!srcwin || !dstwin) return ERR;
 /* Clamp underflowing coords to the top-left. */
 if (dminrow < 0) sminrow += -dminrow,dminrow = 0;
 if (dmincol < 0) smincol += -dmincol,dmincol = 0;
 /* Clamp overflowing coords to the bottom-right. */
 if (dmaxcol > dstwin->w_end.x) dmaxcol = dstwin->w_end.x;
 if (dmaxrow > dstwin->w_end.y) dmaxrow = dstwin->w_end.y;
 /* Check for non-empty copy area. */
 if (dmaxrow < dminrow || dmaxcol < dmincol) return OK;
 sizex = (dmaxcol-dmincol)+1,sizey = (dmaxrow-dminrow)+1;
 scan_src = srcwin->w_dim.x,scan_dst = dstwin->w_dim.x;
 dst = BUFAT(dstwin,dminrow,dmincol);
 if (!overlay) {
  filler = (chtype *)alloca(scan_dst*sizeof(chtype));
  end = (iter = filler)+scan_dst;
  for (; iter != end; ++iter) *iter = srcwin->w_bkgd|' ';
 }

 if (srcwin->w_dim.x <= smincol) goto fill_clear;
 if (srcwin->w_dim.y <= sminrow) goto fill_clear;
 if (smincol < 0) {
  /* Clamp underflow. */
  fill_x_before = -smincol;
  if (fill_x_before >= srcwin->w_dim.x) goto fill_clear;
  if (sizex < fill_x_before) fill_x_before = sizex;
  copy_x = min(sizex,srcwin->w_dim.x)-fill_x_before;
 } else {
  fill_x_before = 0;
  copy_x = min(sizex,srcwin->w_dim.x-sminrow);
 }
 if (sminrow < 0) {
  /* Clamp underflow. */
  fill_y_before = -sminrow;
  if (fill_y_before >= srcwin->w_dim.y) goto fill_clear;
  if (sizey < fill_y_before) fill_y_before = sizey;
  copy_y = min(sizey,srcwin->w_dim.y)-fill_y_before;
 } else {
  fill_y_before = 0;
  copy_y = min(sizey,srcwin->w_dim.y-sminrow);
 }
 assert(copy_x != 0);
 assert(fill_x_before < sizex);
 assert(fill_x_before+copy_x <= sizex);
 assert(copy_y != 0);
 assert(fill_y_before < sizey);
 assert(fill_y_before+copy_y <= sizey);
 fill_x_after = sizex-(fill_x_before+copy_x);
 fill_y_after = sizey-(fill_y_before+copy_y);
 src = BUFAT(srcwin,sminrow+fill_y_before,smincol+fill_x_before);
 if (!overlay) {
  while (fill_y_before--) {
   __waddchnstrat(dstwin,dst,filler,scan_dst);
   dst += scan_dst;
  }
  while (copy_y--) {
   iter = dst;
   __waddchnstrat(dstwin,iter,filler,fill_x_before);
   iter += fill_x_before;
   __waddchnstrat(dstwin,iter,src,copy_x);
   iter += copy_x;
   __waddchnstrat(dstwin,iter,filler,fill_x_after);
   dst += scan_dst;
   src += scan_src;
  }
  while (fill_y_after--) {
   __waddchnstrat(dstwin,dst,filler,scan_dst);
   dst += scan_dst;
  }
 } else {
  dst += fill_x_before+fill_y_before*scan_dst;
  while (copy_y--) {
   __waddchnstrat_overlay(dstwin,dst,src,copy_x);
   dst += scan_dst;
   src += scan_src;
  }
 }
 return OK;
fill_clear:
 /* Completely fill the destination rectangle with background. */
 /* Entire source rectangle is off-screen. */
 if (!overlay) { /* Don't do anything if overlay-mode is enabled. */
  while (sizey--) {
   __waddchnstrat(dstwin,dst,filler,scan_dst);
   dst += scan_dst;
  }
 }
 return OK;
}

#define HBIT_ON(win,x_,y_)  (offset = ((x_)+(y_)*(win)->w_dim.x),(win)->w_text.wt_hidden[offset/8] |= (1 << (offset % 8)))
#define HBIT_OFF(win,x_,y_) (offset = ((x_)+(y_)*(win)->w_dim.x),(win)->w_text.wt_hidden[offset/8] &= ~(1 << (offset % 8)))
static void
win_makeinvisible(WINDOW *win,
                  coord_t begin_x, coord_t begin_y,
                  coord_t size_x, coord_t size_y) {
 struct point iter,end;
 int offset;
 assert(size_x != 0);
 assert(size_y != 0);
 assert(begin_x+size_x <= win->w_dim.x);
 assert(begin_y+size_y <= win->w_dim.y);
 end.x = begin_x+size_x;
 end.y = begin_y+size_y;
 for (iter.y = begin_y; iter.y != end.y; ++iter.y)
 for (iter.x = begin_x; iter.x != end.x; ++iter.x) {
  HBIT_ON(win,iter.x,iter.y);
 }
}
static void win_updatevisible(WINDOW *win) {
 WINDOW **iter,**end,*elem;
 memset(win->w_text.wt_hidden,0,ceildiv(win->w_dim.x*win->w_dim.y,8));
 end = (iter = win->w_childv)+win->w_childc;
 for (; iter != end; ++iter) {
  elem = *iter;
  win_makeinvisible(win,elem->w_ppos.x,elem->w_ppos.y,
                        elem->w_dim.x,elem->w_dim.y);
 }
}



/* Creates a new window at parent-relative coords and the given size.
 * If 'parent' is NULL, create the window as top-z-order window on the
 * current screen, where the given coords will be relative to the screen. */
static WINDOW *mkwin(WINDOW *parent, int nlines, int ncols, int begin_y, int begin_x) {
 WINDOW *result; size_t total_chars;
 assert(nlines > 0),assert(ncols > 0);
 total_chars = nlines*ncols;
 if __unlikely((result = omalloc(WINDOW)) == NULL) goto err_null;
 result->w_text.wt_lines = (struct winline *)malloc(nlines*sizeof(struct winline));
 if __unlikely(!result->w_text.wt_lines) goto err_r;
 result->w_text.wt_textbuf = (chtype *)malloc(total_chars*sizeof(chtype));
 if __unlikely(!result->w_text.wt_textbuf) goto err_lines;
 result->w_text.wt_hidden = (uint8_t *)calloc(1,ceildiv(total_chars,8));
 if __unlikely(!result->w_text.wt_hidden) goto err_textbuf;
 result->w_dim.x = ncols;
 result->w_dim.y = nlines;
 result->w_text.wt_textend = result->w_text.wt_textbuf+(total_chars);
 result->w_ppos.x = begin_x;
 result->w_ppos.y = begin_y;
 if ((result->w_parent = parent) != NULL) {
  WINDOW **new_child_vec;
  new_child_vec = (WINDOW **)realloc(parent->w_childv,
                                    (parent->w_childc+1)*sizeof(WINDOW *));
  if __unlikely(!new_child_vec) goto err_hidden;
  parent->w_childv = new_child_vec;
  new_child_vec[parent->w_childc++] = result;
  result->w_dev     = parent->w_dev;
  result->w_begin.x = parent->w_begin.x+begin_x;
  result->w_begin.y = parent->w_begin.y+begin_y;
  result->w_end.x   = result->w_begin.x+ncols;
  result->w_end.y   = result->w_begin.y+nlines;
  win_makeinvisible(parent,begin_x,begin_y,ncols,nlines);
  win_changedall(parent); /*< TODO: Not all has changed... */
 } else {
  result->w_dev   = __curterm;
  result->w_begin = result->w_ppos;
  result->w_end.x = begin_x+ncols;
  result->w_end.x = begin_y+nlines;
 }
 result->w_bkgd = 0;
 result->w_curpos = result->w_text.wt_textbuf;
 result->w_flags = W_DEFAULT;
 __curterm->d_top = result;
 /* Mark everything in the new window as changed. */
 win_changedall(result);
 return result;
err_hidden: free(result->w_text.wt_hidden);
err_textbuf: free(result->w_text.wt_textbuf);
err_lines: free(result->w_text.wt_lines);
err_r: free(result);
err_null: return NULL;
}

__public WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x) {
 if (nlines <= 0) nlines = LINES;
 if (ncols  <= 0) ncols  = COLS;
 return mkwin(NULL,nlines,ncols,begin_y,begin_x);
}
__public WINDOW *derwin(WINDOW *orig, int nlines, int ncols, int begin_y, int begin_x) {
 if (!orig || nlines <= 0 || ncols <= 0) return NULL;
 return mkwin(orig,nlines,ncols,begin_y,begin_x);
}
__public WINDOW *subwin(WINDOW *orig, int nlines, int ncols, int begin_y, int begin_x) {
 if (!orig || nlines <= 0 || ncols <= 0) return NULL;
 begin_x += orig->w_begin.x;
 begin_y += orig->w_begin.y;
 return mkwin(orig,nlines,ncols,begin_y,begin_x);
}

__public int delwin(WINDOW *win) {
 WINDOW *parent_window,**iter;
 if __unlikely(!win || win->w_childc) return ERR;
 if __unlikely(win == stdsrc) {
  assert(!win->w_parent);
  return endwin();
 }
 assert(!win->w_childv);
 if ((parent_window = win->w_parent) != NULL) {
  /* Remove the window from its parent. */
  iter = parent_window->w_childv;
  for (;; ++iter) {
   assert(iter < parent_window->w_childv+parent_window->w_childc);
   if (*iter == win) break;
  }
  if (!--parent_window->w_childc) {
   free(parent_window->w_childv);
   parent_window->w_childv = NULL;
  } else {
   memmove(iter,iter+1,parent_window->w_childc-
          (iter-parent_window->w_childv));
   iter = (WINDOW **)realloc(parent_window->w_childv,
                             parent_window->w_childc*
                             sizeof(WINDOW *));
   if (iter) parent_window->w_childv = iter;
  }
  win_updatevisible(parent_window);
  win_changedall(parent_window);
 }
 free(win->w_text.wt_lines);
 free(win->w_text.wt_textbuf);
 free(win->w_text.wt_hidden);
 free(win);
 return OK;
}
__public int mvwin(WINDOW *win, int y, int x) {
 if (!win) return ERR;
 if (y == win->w_begin.y && x == win->w_begin.x) return OK;
 win->w_begin.x = x;
 win->w_begin.y = y;
 win->w_end.x = x+win->w_dim.x;
 win->w_end.y = y+win->w_dim.y;
 if (win->w_parent) {
  win->w_ppos.x = win->w_begin.x-win->w_parent->w_begin.x;
  win->w_ppos.y = win->w_begin.y-win->w_parent->w_begin.y;
  /* Force repaint of area previously covered by this window. */
  win_updatevisible(win->w_parent);
  win_changedall(win->w_parent);
 }
 /* Force repaint of window. */
 win_changedall(win);
 return OK;
}
__public int mvderwin(WINDOW *win, int par_y, int par_x) {
 return win->w_parent
  ? mvwin(win,par_y-win->w_parent->w_begin.y,par_x-win->w_parent->w_begin.x)
  : mvwin(win,par_y,par_x);
}
__public WINDOW *dupwin(WINDOW *win) {
 WINDOW *result;
 if __unlikely(!win) return NULL;
 result = mkwin(win->w_parent,win->w_dim.y,win->w_dim.x,
                win->w_ppos.y,win->w_ppos.x);
 if __unlikely(!result) return NULL;
 memcpy(result->w_text.wt_textbuf,win->w_text.wt_textbuf,
        win->w_dim.x*win->w_dim.y*sizeof(chtype));
 result->w_curpos = result->w_text.wt_textbuf+(win->w_curpos-win->w_text.wt_textbuf);
 return result;
}
__public void wsyncup(WINDOW *__unused(win)) {}
__public void wcursyncup(WINDOW *__unused(win)) {}
__public void wsyncdown(WINDOW *__unused(win)) {}
__public int cbreak(void)   { return __curses_setlflag(ICANON,1); }
__public int nocbreak(void) { return __curses_setlflag(ICANON,0); }
__public int keypad(WINDOW *win, bool bf) { 
 if (!win) return ERR;
 (void)bf; /* TODO */
 return OK;
}
__public int raw(void) { return cbreak(); }
__public int noraw(void) { return nocbreak(); }
__public int typeahead(int __unused(fd)) { return OK; }





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
 if (KE_ISERR(ktime_getnow(&abstime))) return ERR;
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
