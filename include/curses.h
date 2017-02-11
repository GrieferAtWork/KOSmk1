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
#ifndef __CURSES_H__
#define __CURSES_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

__DECL_BEGIN

#undef point
#undef curdev
#undef window
#undef coord_t
#undef pair_t
#undef flag_t
#ifndef __BUILTIN_LIBCURSES__
#define point   __cur_point
#define screen  __cur_curdev
#define window  __cur_window
#define coord_t __cur_coord_t
#define pair_t  __cur_pair_t
#define flag_t  __cur_flat_w
#endif

#undef ERR
#undef OK
#define ERR (-1)
#define OK  (0)

#undef TRUE
#undef FALSE
/* As defined by <stdbool.h> */
#define TRUE  true
#define FALSE false

struct window;
typedef uint16_t coord_t;
typedef int16_t scoord_t;
struct point { coord_t x,y; };
struct spoint { scoord_t x,y; };

typedef short    pair_t;
typedef uint32_t chtype; /*< Character + attributes. */
typedef	chtype	  attr_t;		/* ...must be at least as wide as chtype */
typedef uint32_t flag_t;

#define __AC(x) x##u

/* Attributes or'ed together in 'chtype'-style characters. */
#define A_NORMAL       __AC(0x00000000)
#define A_ATTRIBUTES   __AC(0xffffff00)
#define A_CHARTEXT     __AC(0x000000ff)
#define A_COLOR        __AC(0x0000ff00)
#define A_STANDOUT     __AC(0x00010000)
#define A_UNDERLINE    __AC(0x00020000)
#define A_REVERSE      __AC(0x00040000)
#define A_BLINK        __AC(0x00080000)
#define A_DIM          __AC(0x00100000)
#define A_BOLD         __AC(0x00200000)
#define A_ALTCHARSET   __AC(0x00400000)
#define A_INVIS        __AC(0x00800000)
#define A_PROTECT      __AC(0x01000000)
#define A_HORIZONTAL   __AC(0x02000000)
#define A_LEFT         __AC(0x04000000)
#define A_LOW          __AC(0x08000000)
#define A_RIGHT        __AC(0x10000000)
#define A_TOP          __AC(0x20000000)
#define A_VERTICAL     __AC(0x40000000)
#define A_ITALIC       __AC(0x80000000)

#define ACS_VLINE     (A_ALTCHARSET|'x') /* 0x78: | */
#define ACS_HLINE     (A_ALTCHARSET|'q') /* 0x71: - */
#define ACS_ULCORNER  (A_ALTCHARSET|'l') /* 0x6C: /- */
#define ACS_URCORNER  (A_ALTCHARSET|'k') /* 0x6B: -\ */
#define ACS_LLCORNER  (A_ALTCHARSET|'m') /* 0x6D: \- */
#define ACS_LRCORNER  (A_ALTCHARSET|'j') /* 0x6A: -/ */

struct devline {
    /* NOTE: When the following two are equal, they don't have to point into the actual associated line. */
    chtype *dl_chbegin; /*< [?..1][!NULL==wl_chend] Pointer to first character in this line that has changed. */
    chtype *dl_chend;   /*< [?..1][!NULL==wl_chbegin] Pointer to last character in this line that has changed. */
};
struct devtext {
    struct devline *dt_lines;   /*< [1..1][size(:w_dim.y)][owned] Vector of line attributes. */
    chtype         *dt_textbuf; /*< [1..1][owned] Text+attribute vector allocated for this window. */
    chtype         *dt_textend; /*< [1..1][is(wt_textbuf+:w_dim.x*:w_dim.y)] End of the text buffer. */
};

struct screen {
    int            d_ifd;  /*< Terminal input descriptor. */
    int            d_ofd;  /*< Terminal output descriptor. */
    struct point   d_size; /*< Terminal dimensions. */
    struct window *d_top;  /*< [0..1] Top z-ordering window. */
    struct devtext d_scr;  /*< Virtual representation of what has changed on-screen. */
    chtype         d_attr; /*< Text attributes currently set in the terminal. */
};


struct winline {
    /* NOTE: When the following two are equal, they don't have to point into the actual associated line. */
    chtype  *wl_chbegin; /*< [?..1][!NULL==wl_chend] Pointer to first character in this line that has changed. */
    chtype  *wl_chend;   /*< [?..1][!NULL==wl_chbegin] Pointer to last character in this line that has changed. */
};
struct wintext {
    struct winline *wt_lines;   /*< [1..1][size(:w_dim.y)][owned] Vector of line attributes. */
    chtype         *wt_textbuf; /*< [1..1][owned] Text+attribute vector allocated for this window. */
    chtype         *wt_textend; /*< [1..1][is(wt_textbuf+:w_dim.x*:w_dim.y)] End of the text buffer. */
    uint8_t        *wt_hidden;  /*< [1..1][owned] Bit-mask of all hidden characters (in MSB order; aka. '1 << (offset % 8)'-style). */
};

#define __CURSES_WINDOWFLAG_NONE    0x00000000
#define __CURSES_WINDOWFLAG_SCRLOK  0x00000001 /*< [0] Scroll-lock is enabled. */
#define __CURSES_WINDOWFLAG_CLROK   0x00000002 /*< [0] Clear the window completely on the next call to wrefresh(). */
#define __CURSES_WINDOWFLAG_IDLOK   0x00000004 /*< [0] Attempt to use hardward-support for line insert/erase. */
#define __CURSES_WINDOWFLAG_IDCOK   0x00000008 /*< [1] Attempt to use hardward-support for character insert/erase. */
#define __CURSES_WINDOWFLAG_LEAVEOK 0x00000010 /*< [0] The hardware cursor can be left where-ever. */
#define __CURSES_WINDOWFLAG_IMMEDOK 0x00000020 /*< [0] Immediately mirror changes on-screen. */

struct window {
    struct window *w_zprev;  /*< [0..1] Nearest window with a lower z-order. */
    struct window *w_znext;  /*< [0..1] Nearest window with a greater z-order. */
    struct screen *w_dev;    /*< [1..1] Associated curses terminal device. */
    struct window *w_parent; /*< [0..1] Parent of this window. */
    struct spoint  w_ppos;   /*< Window position in parent window (== {0,0} if no parent is set). */
    struct spoint  w_begin;  /*< Start coords of this window (in w_dev). */
    struct spoint  w_end;    /*< End coords of this window (in w_dev). */
    struct point   w_dim;    /*< Window dimensions (== w_end-w_begin) */
    struct wintext w_text;   /*< Window text buffer. */
    chtype        *w_curpos; /*< [1..1][in(w_text.wl_textbuf..w_text.wl_textend)] Cursor in the text buffer. */
    chtype         w_bkgd;   /*< Background character & attributes. */
    flag_t         w_flags;  /*< Special window flags (Set of 'W_*' flags). */
};

#ifdef __BUILTIN_LIBCURSES__
#define W_NONE    __CURSES_WINDOWFLAG_NONE
#define W_SCRLOK  __CURSES_WINDOWFLAG_SCRLOK
#define W_CLROK   __CURSES_WINDOWFLAG_CLROK
#define W_IDLOK   __CURSES_WINDOWFLAG_IDLOK
#define W_IDCOK   __CURSES_WINDOWFLAG_IDCOK
#define W_LEAVEOK __CURSES_WINDOWFLAG_LEAVEOK
#define W_IMMEDOK __CURSES_WINDOWFLAG_IMMEDOK
#endif

typedef struct screen SCREEN;
typedef struct window WINDOW;

#define __cur_getcuroff(win) ((win)->w_curpos-(win)->w_text.wt_textbuf)

#define getattrs(win) ((win) ? (win)->w_attr : ERR)
#define getcurx(win)  ((win) ? (int)(__cur_getcuroff(win)%(win)->w_dim.x) : ERR)
#define getcury(win)  ((win) ? (int)(__cur_getcuroff(win)/(win)->w_dim.x) : ERR)
#define getbegx(win)  ((win) ? (int)(win)->w_begin.x : ERR)
#define getbegy(win)  ((win) ? (int)(win)->w_begin.y : ERR)
#define getmaxx(win)  ((win) ? (int)(win)->w_end.x : ERR)
#define getmaxy(win)  ((win) ? (int)(win)->w_end.y : ERR)
#define getparx(win)  ((win) ? (int)(win)->w_ppos.x : ERR)
#define getpary(win)  ((win) ? (int)(win)->w_ppos.y : ERR)

#define getyx(win,y,x)    ((y) = getcury(win),(x) = getcurx(win))
#define getbegyx(win,y,x) ((y) = getbegy(win),(x) = getbegx(win))
#define getmaxyx(win,y,x) ((y) = getmaxy(win),(x) = getmaxx(win))
#define getparyx(win,y,x) ((y) = getpary(win),(x) = getparx(win))

#define wgetparent(win) ((win) ? (win)->w_parent : NULL)

extern SCREEN   *__curterm;
extern SCREEN    __stdterm;
extern WINDOW    __stdsrc_;
#define stdsrc (&__stdsrc_)

#define COLS   (__curterm->d_size.x)
#define LINES  (__curterm->d_size.y)

/* refresh curses windows and lines. */
extern int wnoutrefresh(WINDOW *win);
extern int doupdate(void);
extern int wrefresh(WINDOW *win);
extern int wredrawln(WINDOW *win, int beg_line, int num_lines);
extern int redrawwin(WINDOW *win);
extern int __ddoupdate(struct screen *dev);
#define refresh()  wrefresh(stdsrc)
#ifndef __INTELLISENSE__
#if 1
#define doupdate() __ddoupdate(__curterm)
#endif
#endif

/* move curses window cursor. */
extern int wmove(WINDOW *win, int y, int x);
#ifdef __INTELLISENSE__
extern int move(int y, int x);
#else
#define move(y,x) wmove(stdsrc,y,x)
#endif

/* curses output options. */
extern int idlok(WINDOW *win, bool bf);
extern void immedok(WINDOW *win, bool bf);
extern int wsetscrreg(WINDOW *win, int top, int bot);
#ifdef __INTELLISENSE__
extern int clearok(WINDOW *win, bool bf);
extern void idcok(WINDOW *win, bool bf);
extern int leaveok(WINDOW *win, bool bf);
extern int setscrreg(int top, int bot);
extern int scrollok(WINDOW *win, bool bf);
extern int nl(void);
extern int nonl(void);
#else
#define __curses_wsetflag(win,bf,flag) \
 ((win) ? ((bf) ? ((win)->w_flags |= (flag),OK) : ((win)->w_flags &= ~(flag),OK)) : ERR)
#define clearok(win,bf)          __curses_wsetflag(win,bf,__CURSES_WINDOWFLAG_CLROK)
#define idcok(win,bf)      (void)__curses_wsetflag(win,bf,__CURSES_WINDOWFLAG_IDCOK)
#define leaveok(win,bf)          __curses_wsetflag(win,bf,__CURSES_WINDOWFLAG_LEAVEOK)
#define setscrreg(top,bot)              wsetscrreg(stdsrc,top,bot)
#define scrollok(win,bf)         __curses_wsetflag(win,bf,__CURSES_WINDOWFLAG_SCRLOK)
#define nl()
#define nonl()
#endif

/* clear all or part of a curses window. */
extern int wclrtobot(WINDOW *win);
extern int wclrtoeol(WINDOW *win);
#ifdef __INTELLISENSE__
extern int erase(void);
extern int werase(WINDOW *win);
extern int clear(void);
extern int wclear(WINDOW *win);
extern int clrtobot(void);
extern int clrtoeol(void);
#else
#define erase()      werase(stdsrc)
#define werase(win) (wmove(win,0,0) == OK ? wclrtobot(win) : ERR)
#define clear()      clear(stdsrc)
#define wclear(win) (wmove(win,0,0) == OK && wclrtobot(win) == OK ? clearok(win,TRUE) : ERR)
#define clrtobot()   wclrtobot(stdsrc)
#define clrtoeol()   wclrtoeol(stdsrc)
#endif


/* scroll a curses window. */
extern int wscrl(WINDOW *win, int n);
#ifdef __INTELLISENSE__
extern int scroll(WINDOW *win);
extern int scrl(int n);
#else
#define scroll(win) wscrl(win,1)
#define scrl(n)     wscrl(stdsrc,n)
#endif

/* add a character (with attributes) to a curses window, then advance the cursor. */
extern int waddch(WINDOW *win, chtype const ch);
extern int wechochar(WINDOW *win, chtype const ch);
#ifdef __INTELLISENSE__
extern int addch(chtype const ch);
extern int mvaddch(int y, int x, chtype const ch);
extern int mvwaddch(WINDOW *win, int y, int x, chtype const ch);
extern int echochar(chtype const ch);
#else
#define addch(ch)             waddch(stdsrc,ch)
#define mvaddch(y,x,ch)       mvwaddch(stdsrc,y,x,ch)
#define mvwaddch(win,y,x,ch) (wmove(win,y,x) == OK ? waddch(win,ch) : ERR)
#define echochar(ch)          wechochar(stdsrc,ch)
#endif

/* screen initialization and manipulation routines. */
extern WINDOW *initscr(void);
extern int endwin(void);
extern bool isendwin(void);
extern SCREEN *newterm(char *type, FILE *outfd, FILE *infd);
extern SCREEN *set_term(SCREEN *new_);
extern void delscreen(SCREEN* sp);

/* create curses borders, horizontal and vertical lines. */
extern int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr, chtype bl, chtype br);
extern int whline(WINDOW *win, chtype ch, int n);
extern int wvline(WINDOW *win, chtype ch, int n);
extern int mvwhline(WINDOW *win, int y, int x, chtype ch, int n);
extern int mvwvline(WINDOW *win, int y, int x, chtype ch, int n);
extern int hline(chtype ch, int n);
extern int vline(chtype ch, int n);
extern int mvhline(int y, int x, chtype ch, int n);
extern int mvvline(int y, int x, chtype ch, int n);
#ifdef __INTELLISENSE__
extern int border(chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr, chtype bl, chtype br);
extern int box(WINDOW *win, chtype verch, chtype horch);
#else
#define mvwhline(win,y,x,ch,n)         (wmove(win,y,x) == OK ? whline(win,ch,n) : ERR)
#define mvwvline(win,y,x,ch,n)         (wmove(win,y,x) == OK ? wvline(win,ch,n) : ERR)
#define hline(ch,n)                     whline(stdsrc,ch,n)
#define vline(ch,n)                     wvline(stdsrc,ch,n)
#define mvhline(y,x,ch,n)               mvwhline(stdsrc,y,x,ch,n)
#define mvvline(y,x,ch,n)               mvwvline(stdsrc,y,x,ch,n)
#define border(ls,rs,ts,bs,tl,tr,bl,br) wborder(stdsrc,ls,rs,ts,bs,tl,tr,bl,br)
#define box(win,verch,horch)            wborder(win,verch,verch,horch,horch,0,0,0,0)
#endif

/* curses window background manipulation routines. */
extern void wbkgdset(WINDOW *win, chtype ch);
extern int wbkgd(WINDOW *win, chtype ch);
extern chtype getbkgd(WINDOW *win);
#ifdef __INTELLISENSE__
extern void bkgdset(chtype ch);
extern int bkgd(chtype ch);
#else
#define bkgdset(ch) wbkgdset(stdsrc,ch)
#define bkgd(ch)    wbkgd(stdsrc,ch)
#endif

/* add a string of characters (and attributes) to a curses window. */
extern int waddchnstr(WINDOW *win, chtype const *chstr, int n);
#ifdef __INTELLISENSE__
extern int addchstr(chtype const *chstr);
extern int addchnstr(chtype const *chstr, int n);
extern int waddchstr(WINDOW *win, chtype const *chstr);
extern int mvaddchstr(int y, int x, chtype const *chstr);
extern int mvaddchnstr(int y, int x, chtype const *chstr, int n);
extern int mvwaddchstr(WINDOW *win, int y, int x, chtype const *chstr);
extern int mvwaddchnstr(WINDOW *win, int y, int x, chtype const *chstr, int n);
#else
#define addchstr(chstr)                waddchstr(stdsrc,chstr)
#define addchnstr(chstr,n)             waddchnstr(stdsrc,chstr,n)
#define waddchstr(win,chstr)           waddchnstr(win,chstr,-1)
#define mvaddchstr(y,x,chstr)          mvwaddchstr(stdsrc,y,x,chstr)
#define mvaddchnstr(y,x,chstr,n)       mvwaddchnstr(stdsrc,y,x,chstr,n)
#define mvwaddchstr(win,y,x,chstr)     mvwaddchnstr(win,y,x,chstr,-1)
#define mvwaddchnstr(win,y,x,chstr,n) (wmove(win,y,x) == OK ? waddchnstr(win,chstr,n) : ERR)
#endif

/* add a string of characters to a curses window and advance cursor. */
extern int waddstr(WINDOW *win, const char *str);
extern int waddnstr(WINDOW *win, const char *str, int n);
#ifdef __INTELLISENSE__
extern int addstr(const char *str);
extern int addnstr(const char *str, int n);
extern int mvaddstr(int y, int x, const char *str);
extern int mvaddnstr(int y, int x, const char *str, int n);
extern int mvwaddstr(WINDOW *win, int y, int x, const char *str);
extern int mvwaddnstr(WINDOW *win, int y, int x, const char *str, int n);
#else
#define addstr(str)                waddstr(stdsrc,str)
#define addnstr(str,n)             waddnstr(stdsrc,str,n)
#define mvaddstr(y,x,str)          mvwaddstr(stdsrc,y,x,str)
#define mvaddnstr(y,x,str,n)       mvwaddnstr(stdsrc,y,x,str,n)
#define mvwaddstr(win,y,x,str)    (wmove(win,y,x) == OK ? waddstr(win,str) : ERR)
#define mvwaddnstr(win,y,x,str,n) (wmove(win,y,x) == OK ? waddnstr(win,str,n) : ERR)
#endif

/* print formatted output in curses windows. */
extern __attribute_vaformat(__printf__,1,2) int printw(const char *fmt, ...);
extern __attribute_vaformat(__printf__,2,3) int wprintw(WINDOW *win, const char *fmt, ...);
extern __attribute_vaformat(__printf__,3,4) int mvprintw(int y, int x, const char *fmt, ...);
extern __attribute_vaformat(__printf__,4,5) int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);
extern __attribute_vaformat(__printf__,2,0) int vwprintw(WINDOW *win, const char *fmt, va_list varglist);
extern __attribute_vaformat(__printf__,2,0) int vw_printw(WINDOW *win, const char *fmt, va_list varglist);

extern int curs_set(int visibility);
extern int napms(int ms); /*< Seems like every library has one of these... */
extern int beep(void);
extern int flash(void);

#if 0
extern int copywin(WINDOW const *oldwin, WINDOW *newwin,int,int,int,int,int,int,int);
extern int delwin(WINDOW *win);
extern WINDOW *derwin(WINDOW *win, int,int,int,int);
extern WINDOW *dupwin(WINDOW *win);

extern int intrflush(WINDOW *win, bool);
extern bool is_linetouched(WINDOW *win, int);
extern bool is_wintouched(WINDOW *win);
extern int keypad(WINDOW *win, bool);
extern int meta(WINDOW *win, bool);
extern int mvderwin(WINDOW *win, int, int);
extern int mvwchgat(WINDOW *win, int, int, int, attr_t, pair_t, const void *);
extern int mvwdelch(WINDOW *win, int, int);
extern int mvwgetch(WINDOW *win, int, int);
extern int mvwgetnstr(WINDOW *win, int, int, char *, int);
extern int mvwgetstr(WINDOW *win, int, int, char *);
extern int mvwin(WINDOW *win, int,int);
extern chtype mvwinch(WINDOW *win, int, int);
extern int mvwinchnstr(WINDOW *win, int, int, chtype *, int);
extern int mvwinchstr(WINDOW *win, int, int, chtype *);
extern int mvwinnstr(WINDOW *win, int, int, char *, int);
extern int mvwinsch(WINDOW *win, int, int, chtype);
extern int mvwinsnstr(WINDOW *win, int, int, const char *, int);
extern int mvwinsstr(WINDOW *win, int, int, const char *);
extern int mvwinstr(WINDOW *win, int, int, char *);
extern __attribute_vaformat(__scanf__,4,5) int mvwscanw(WINDOW *win, int,int, char const *, ...);
extern WINDOW *newpad (int,int);
extern WINDOW *newwin (int,int,int,int);
extern int nodelay(WINDOW *win, bool);
extern int notimeout(WINDOW *win, bool);
extern int overlay (WINDOW const *oldwin, WINDOW *newwin);
extern int overwrite (WINDOW const *oldwin, WINDOW *newwin);
extern int pechochar(WINDOW *win, chtype const);
extern int pnoutrefresh (WINDOW *win, int,int,int,int,int,int);
extern int prefresh(WINDOW *win, int,int,int,int,int,int);
extern int ripoffline (int, int (*)(WINDOW *win, int));
extern WINDOW *subpad(WINDOW *win, int, int, int, int);
extern WINDOW *subwin(WINDOW *win, int, int, int, int);
extern int syncok(WINDOW *win, bool);
extern int touchline(WINDOW *win, int, int);
extern int touchwin(WINDOW *win);
extern int untouchwin(WINDOW *win);
extern int vwscanw(WINDOW *win, const char *,va_list);
extern int vw_scanw(WINDOW *win, const char *,va_list);
extern int wattron(WINDOW *win, int);
extern int wattroff(WINDOW *win, int);
extern int wattrset(WINDOW *win, int);
extern int wattr_get(WINDOW *win, attr_t *, pair_t *, void *);
extern int wattr_on(WINDOW *win, attr_t, void *);
extern int wattr_off(WINDOW *win, attr_t, void *);
extern int wattr_set(WINDOW *win, attr_t, pair_t, void *);
extern int wchgat(WINDOW *win, int, attr_t, pair_t, const void *);
extern int wcolor_set (WINDOW *win, pair_t,void*);
extern void wcursyncup(WINDOW *win);
extern int wdelch(WINDOW *win);
extern int wdeleteln(WINDOW *win);
extern int wgetch(WINDOW *win);
extern int wgetnstr(WINDOW *win, char *,int);
extern int wgetstr(WINDOW *win, char *);
extern chtype winch(WINDOW *win);
extern int winchnstr(WINDOW *win, chtype *, int);
extern int winchstr(WINDOW *win, chtype *);
extern int winnstr(WINDOW *win, char *, int);
extern int winsch(WINDOW *win, chtype);
extern int winsdelln(WINDOW *win, int);
extern int winsertln(WINDOW *win);
extern int winsnstr(WINDOW *win, const char *,int);
extern int winsstr(WINDOW *win, const char *);
extern int winstr(WINDOW *win, char *);
extern __attribute_vaformat(__scanf__,2,3) int wscanw(WINDOW *win, const char *,...);
extern int wstandout(WINDOW *win);
extern int wstandend(WINDOW *win);
extern void wsyncdown(WINDOW *win);
extern void wsyncup(WINDOW *win);
extern void wtimeout(WINDOW *win, int);
extern int wtouchln(WINDOW *win, int,int,int);
extern int wresize(WINDOW *win, int, int);
#endif



/*
extern bool is_cleared (const WINDOW *win);
extern bool is_idcok (const WINDOW *win);
extern bool is_idlok (const WINDOW *win);
extern bool is_immedok (const WINDOW *win);
extern bool is_keypad (const WINDOW *win);
extern bool is_leaveok (const WINDOW *win);
extern bool is_nodelay (const WINDOW *win);
extern bool is_notimeout (const WINDOW *win);
extern bool is_pad (const WINDOW *win);
extern bool is_scrollok (const WINDOW *win);
extern bool is_subwin (const WINDOW *win);
extern bool is_syncok (const WINDOW *win);
extern int wgetdelay (const WINDOW *win);
extern int wgetscrreg (const WINDOW *win, int *, int *);
*/


#ifndef __BUILTIN_LIBCURSES__
#undef point
#undef curdev
#undef window
#undef coord_t
#undef pair_t
#undef flag_t
#endif

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__CURSES_H__ */
