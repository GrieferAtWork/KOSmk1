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
#ifndef __TERMINAL_VGA_C__
#define __TERMINAL_VGA_C__ 1

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <kos/arch/x86/vga.h>
#include <kos/atomic.h>
#include <kos/keyboard.h>
#include <kos/syslog.h>
#include <kos/time.h>
#include <proc.h>
#include <limits.h>
#include <math.h>
#include <pty.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <term/term.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static __u8 vga_invert[VGA_COLORS] = {
#ifndef __INTELLISENSE__
 [VGA_COLOR_BLACK         ] = VGA_COLOR_WHITE,
 [VGA_COLOR_BLUE          ] = VGA_COLOR_LIGHT_BROWN,
 [VGA_COLOR_GREEN         ] = VGA_COLOR_LIGHT_MAGENTA,
 [VGA_COLOR_CYAN          ] = VGA_COLOR_LIGHT_RED,
 [VGA_COLOR_RED           ] = VGA_COLOR_LIGHT_CYAN,
 [VGA_COLOR_MAGENTA       ] = VGA_COLOR_LIGHT_GREEN,
 [VGA_COLOR_BROWN         ] = VGA_COLOR_LIGHT_BLUE,
 [VGA_COLOR_LIGHT_GREY    ] = VGA_COLOR_DARK_GREY,
 [VGA_COLOR_DARK_GREY     ] = VGA_COLOR_LIGHT_GREY,
 [VGA_COLOR_LIGHT_BLUE    ] = VGA_COLOR_BROWN,
 [VGA_COLOR_LIGHT_GREEN   ] = VGA_COLOR_MAGENTA,
 [VGA_COLOR_LIGHT_CYAN    ] = VGA_COLOR_RED,
 [VGA_COLOR_LIGHT_RED     ] = VGA_COLOR_CYAN,
 [VGA_COLOR_LIGHT_MAGENTA ] = VGA_COLOR_GREEN,
 [VGA_COLOR_LIGHT_BROWN   ] = VGA_COLOR_BLUE,
 [VGA_COLOR_WHITE         ] = VGA_COLOR_BLACK,
#endif
};
static struct term_rgba vga_colors[VGA_COLORS] = {
#ifndef __INTELLISENSE__
#if 0 /* Perfect match for xterm 256 */
 [VGA_COLOR_BLACK         ] = TERM_RGBA_INIT(0x00,0x00,0x00,0xff), // Black
 [VGA_COLOR_BLUE          ] = TERM_RGBA_INIT(0x00,0x00,0x80,0xff), // Navy
 [VGA_COLOR_GREEN         ] = TERM_RGBA_INIT(0x00,0x80,0x00,0xff), // Green
 [VGA_COLOR_CYAN          ] = TERM_RGBA_INIT(0x00,0x80,0x80,0xff), // Teal
 [VGA_COLOR_RED           ] = TERM_RGBA_INIT(0x80,0x00,0x00,0xff), // Maroon
 [VGA_COLOR_MAGENTA       ] = TERM_RGBA_INIT(0x80,0x00,0x80,0xff), // Purple
 [VGA_COLOR_BROWN         ] = TERM_RGBA_INIT(0x80,0x80,0x00,0xff), // Olive
 [VGA_COLOR_LIGHT_GREY    ] = TERM_RGBA_INIT(0xc0,0xc0,0xc0,0xff), // Silver
 [VGA_COLOR_DARK_GREY     ] = TERM_RGBA_INIT(0x80,0x80,0x80,0xff), // Grey
 [VGA_COLOR_LIGHT_BLUE    ] = TERM_RGBA_INIT(0x00,0x00,0xff,0xff), // Blue
 [VGA_COLOR_LIGHT_GREEN   ] = TERM_RGBA_INIT(0x00,0xff,0x00,0xff), // Lime
 [VGA_COLOR_LIGHT_CYAN    ] = TERM_RGBA_INIT(0x00,0xff,0xff,0xff), // Aqua
 [VGA_COLOR_LIGHT_RED     ] = TERM_RGBA_INIT(0xff,0x00,0x00,0xff), // Red
 [VGA_COLOR_LIGHT_MAGENTA ] = TERM_RGBA_INIT(0xff,0x00,0xff,0xff), // Fuchsia
 [VGA_COLOR_LIGHT_BROWN   ] = TERM_RGBA_INIT(0xff,0xff,0x00,0xff), // Yellow
 [VGA_COLOR_WHITE         ] = TERM_RGBA_INIT(0xff,0xff,0xff,0xff), // White
#elif 0 /* Actual colors used by QEMU */
 [VGA_COLOR_BLACK         ] = TERM_RGBA_INIT(0x00,0x00,0x00,0xff), // Black
 [VGA_COLOR_BLUE          ] = TERM_RGBA_INIT(0x00,0x00,0xa8,0xff), // Navy
 [VGA_COLOR_GREEN         ] = TERM_RGBA_INIT(0x00,0xa8,0x00,0xff), // Green
 [VGA_COLOR_CYAN          ] = TERM_RGBA_INIT(0x00,0xa8,0xa8,0xff), // Teal
 [VGA_COLOR_RED           ] = TERM_RGBA_INIT(0xa8,0x00,0x00,0xff), // Maroon
 [VGA_COLOR_MAGENTA       ] = TERM_RGBA_INIT(0xa8,0x00,0xa8,0xff), // Purple
 [VGA_COLOR_BROWN         ] = TERM_RGBA_INIT(0xa8,0x57,0x00,0xff), // Olive
 [VGA_COLOR_LIGHT_GREY    ] = TERM_RGBA_INIT(0xa8,0xa8,0xa8,0xff), // Silver
 [VGA_COLOR_DARK_GREY     ] = TERM_RGBA_INIT(0x57,0x57,0x57,0xff), // Grey
 [VGA_COLOR_LIGHT_BLUE    ] = TERM_RGBA_INIT(0x57,0x57,0xff,0xff), // Blue
 [VGA_COLOR_LIGHT_GREEN   ] = TERM_RGBA_INIT(0x57,0xff,0x57,0xff), // Lime
 [VGA_COLOR_LIGHT_CYAN    ] = TERM_RGBA_INIT(0x57,0xff,0xff,0xff), // Aqua
 [VGA_COLOR_LIGHT_RED     ] = TERM_RGBA_INIT(0xff,0x57,0x57,0xff), // Red
 [VGA_COLOR_LIGHT_MAGENTA ] = TERM_RGBA_INIT(0xff,0x57,0xff,0xff), // Fuchsia
 [VGA_COLOR_LIGHT_BROWN   ] = TERM_RGBA_INIT(0xff,0xff,0x57,0xff), // Yellow
 [VGA_COLOR_WHITE         ] = TERM_RGBA_INIT(0xff,0xff,0xff,0xff), // White
#elif 1 /* Aproximation for best results (Use this!) */
 [VGA_COLOR_BLACK         ] = TERM_RGBA_INIT(0x00,0x00,0x00,0xff), // Black
 [VGA_COLOR_BLUE          ] = TERM_RGBA_INIT(0x00,0x00,0xAA,0xff), // Navy
 [VGA_COLOR_GREEN         ] = TERM_RGBA_INIT(0x00,0xAA,0x00,0xff), // Green
 [VGA_COLOR_CYAN          ] = TERM_RGBA_INIT(0x00,0xAA,0xAA,0xff), // Teal
 [VGA_COLOR_RED           ] = TERM_RGBA_INIT(0xAA,0x00,0x00,0xff), // Maroon
 [VGA_COLOR_MAGENTA       ] = TERM_RGBA_INIT(0xAA,0x00,0xAA,0xff), // Purple
 [VGA_COLOR_BROWN         ] = TERM_RGBA_INIT(0xAA,0xAA,0x00,0xff), // Olive
 [VGA_COLOR_LIGHT_GREY    ] = TERM_RGBA_INIT(0xAA,0xAA,0xAA,0xff), // Silver
 [VGA_COLOR_DARK_GREY     ] = TERM_RGBA_INIT(0x80,0x80,0x80,0xff), // Grey
 [VGA_COLOR_LIGHT_BLUE    ] = TERM_RGBA_INIT(0x00,0x00,0xff,0xff), // Blue
 [VGA_COLOR_LIGHT_GREEN   ] = TERM_RGBA_INIT(0x00,0xff,0x00,0xff), // Lime
 [VGA_COLOR_LIGHT_CYAN    ] = TERM_RGBA_INIT(0x00,0xff,0xff,0xff), // Aqua
 [VGA_COLOR_LIGHT_RED     ] = TERM_RGBA_INIT(0xff,0x00,0x00,0xff), // Red
 [VGA_COLOR_LIGHT_MAGENTA ] = TERM_RGBA_INIT(0xff,0x00,0xff,0xff), // Fuchsia
 [VGA_COLOR_LIGHT_BROWN   ] = TERM_RGBA_INIT(0xff,0xff,0x00,0xff), // Yellow
 [VGA_COLOR_WHITE         ] = TERM_RGBA_INIT(0xff,0xff,0xff,0xff), // White
#else /* Color codes according to VGA? (I think...) */
 [VGA_COLOR_BLACK         ] = TERM_RGBA_INIT(0x00,0x00,0x00,0xff),
 [VGA_COLOR_BLUE          ] = TERM_RGBA_INIT(0x00,0x00,0xaa,0xff),
 [VGA_COLOR_GREEN         ] = TERM_RGBA_INIT(0x00,0xaa,0x00,0xff),
 [VGA_COLOR_CYAN          ] = TERM_RGBA_INIT(0x00,0xaa,0xaa,0xff),
 [VGA_COLOR_RED           ] = TERM_RGBA_INIT(0xaa,0x00,0x00,0xff),
 [VGA_COLOR_MAGENTA       ] = TERM_RGBA_INIT(0xaa,0x00,0xaa,0xff),
 [VGA_COLOR_BROWN         ] = TERM_RGBA_INIT(0xaa,0x55,0x00,0xff),
 [VGA_COLOR_LIGHT_GREY    ] = TERM_RGBA_INIT(0xaa,0xaa,0xaa,0xff),
 [VGA_COLOR_DARK_GREY     ] = TERM_RGBA_INIT(0x80,0x55,0x55,0xff),
 [VGA_COLOR_LIGHT_BLUE    ] = TERM_RGBA_INIT(0x55,0x55,0xff,0xff),
 [VGA_COLOR_LIGHT_GREEN   ] = TERM_RGBA_INIT(0x55,0xaa,0x55,0xff),
 [VGA_COLOR_LIGHT_CYAN    ] = TERM_RGBA_INIT(0x55,0xff,0xff,0xff),
 [VGA_COLOR_LIGHT_RED     ] = TERM_RGBA_INIT(0xff,0x55,0x55,0xff),
 [VGA_COLOR_LIGHT_MAGENTA ] = TERM_RGBA_INIT(0xff,0x55,0xff,0xff),
 [VGA_COLOR_LIGHT_BROWN   ] = TERM_RGBA_INIT(0xff,0xff,0x55,0xff),
 [VGA_COLOR_WHITE         ] = TERM_RGBA_INIT(0xff,0xff,0xff,0xff),
#endif
#endif
};

/* UGH... Doing this correctly would have waaaay too huge of an overhead.
   But I have to admit that this approximation is pretty CRAP... */
#if 0
#define rgba_distance(a,b) (unsigned int)\
 (sqr(((int)(b).sr-(int)(a).sr)*30)+\
  sqr(((int)(b).sg-(int)(a).sg)*59)+\
  sqr(((int)(b).sb-(int)(a).sb)*11))
#elif 1
#define rgba_distance(a,b) (unsigned int)\
 (sqr((a).sr-(b).sr)+\
  sqr((a).sg-(b).sg)+\
  sqr((a).sb-(b).sb))
#else
#define rgba_distance(a,b) (unsigned int)\
 (abs((a).sr-(b).sr)+\
  abs((a).sg-(b).sg)+\
  abs((a).sb-(b).sb))
#endif


__local __u8 vga_color(struct term_rgba const cl) {
 unsigned int d,winner = UINT_MAX;
 __u8 i = 0,result = 0;
 for (; i < VGA_COLORS; ++i) {
  d = rgba_distance(cl,vga_colors[i]);
  if (d < winner) result = i,winner = d;
 }
#if 0
 k_syslogf(KLOG_MSG,
           "COLOR: %u {%I8x,%I8x,%I8x} -> {%I8x,%I8x,%I8x}\n",
           winner,cl.r,cl.g,cl.b,
           vga_colors[result].r,
           vga_colors[result].g,
           vga_colors[result].b);
#endif
 return result;
}


#define INVERT_CURSOR_AFTER_MOVE 1
typedef uint16_t cell_t;

// user-level mappings of the VGA physical memory.
// NOTE: This memory should be considered write-only.
static cell_t *vga_dev;
static cell_t *vga_buf,*vga_bufpos,*vga_bufend;
static cell_t *vga_bufsecondline,*vga_lastline;
#define vga_spaceline   vga_bufend
static cell_t  vga_attrib;
#define VGA_SIZE   (VGA_WIDTH*VGA_HEIGHT)
#define CHR(x)    ((cell_t)(unsigned char)(x)|vga_attrib)
#define SPACE       CHR(' ')
#define INVCHR(x) ((cell_t)(unsigned char)(x)\
   | ((cell_t)vga_invert[(vga_attrib&0xf000) >> 12] << 12)\
   | ((cell_t)vga_invert[(vga_attrib&0x0f00) >> 8] << 8))


// Cell/Line access
#define DEV_CELL(x,y)  (vga_dev+(x)+(y)*VGA_WIDTH)
#define BUF_CELL(x,y)  (vga_buf+(x)+(y)*VGA_WIDTH)
#define DEV_LINE(y)    (vga_dev+(y)*VGA_WIDTH)
#define BUF_LINE(y)    (vga_buf+(y)*VGA_WIDTH)
#define DEV_CELL_CUR() (vga_dev+(vga_bufpos-vga_buf))
#define BUF_CELL_CUR() (vga_bufpos)

// Blit commands (swap buffers)
#define BLIT_CNT(start,count)  memcpy(vga_dev+(start),vga_buf+(start),(count)*sizeof(cell_t))
#define BLIT()                 BLIT_CNT(0,VGA_SIZE)
#define BLIT_LINE(y)           memcpy(DEV_LINE(y),BUF_LINE(y),VGA_WIDTH*sizeof(cell_t))
#define BLIT_CELL(x,y)        (*DEV_CELL(x,y) = *BUF_CELL(x,y))
#define BLIT_CUR()            (*DEV_CELL_CUR() = *BUF_CELL_CUR())
#define BLIT_BUF(buf)         (vga_dev[(buf)-vga_buf] = *(buf))
#define BLIT_CUR_INV()        (*DEV_CELL_CUR() = INVERT(*BUF_CELL_CUR()))

// Get/Set/Move the cursor
#define GET_CUR_X()    (coord_t)((vga_bufpos-vga_buf)%VGA_WIDTH)
#define GET_CUR_Y()    (coord_t)((vga_bufpos-vga_buf)/VGA_WIDTH)
#define SET_CUR(x,y)   (vga_bufpos = vga_buf+((x)+(y)*VGA_WIDTH))
#define SET_CUR_X(x)   (vga_bufpos -= (((vga_bufpos-vga_buf) % VGA_WIDTH)-(x)))
#define SET_CUR_Y(y)    SET_CUR(GET_CUR_X(),y)
#define MOV_CUR(ox,oy) (vga_bufpos += ((ox)+(oy)*VGA_WIDTH))
#define MOV_CUR_X(ox)  (vga_bufpos += (ox))
#define MOV_CUR_Y(oy)  (vga_bufpos += (ox)*VGA_WIDTH)
#define CUR_LINE()     (vga_bufpos-((vga_bufpos-vga_buf) % VGA_WIDTH))

#define CR()            SET_CUR_X(0)
#define LF()           (vga_bufpos += (VGA_WIDTH-((vga_bufpos-vga_buf) % VGA_WIDTH)))
#define BACK()         (vga_bufpos != vga_buf ? --vga_bufpos : vga_bufpos)

static          int relay_incoming_thread;
static          int relay_slave_out_thread;
static          int cursor_blink_thread;
static __atomic int cursor_blink_enabled = 1;
static          int cursor_inverted = 0;
static __atomic int cursor_lock = 0;
#define CURSOR_LOCK   while (!katomic_cmpxch(cursor_lock,0,1)) task_yield();
#define CURSOR_UNLOCK cursor_lock = 0;


static void cursor_enable(void) {
 if (!katomic_cmpxch(cursor_blink_enabled,0,1)) return;
 task_resume(cursor_blink_thread);
}
static void cursor_disable(void) {
 if (!katomic_cmpxch(cursor_blink_enabled,1,0)) return;
 task_suspend(cursor_blink_thread);
 if (cursor_inverted) {
  // Fix inverted cursor
  BLIT_CUR();
  cursor_inverted = 0;
 }
}

#define BEGIN_MOVE_CUR()      { CURSOR_LOCK if (cursor_inverted) BLIT_CUR(); }
#define END_MOVE_CUR()        { CURSOR_UNLOCK }

#define INVERT(cell) \
 (((cell)&0xff)\
   | ((cell_t)vga_invert[((cell)&0xf000) >> 12] << 12)\
   | ((cell_t)vga_invert[((cell)&0x0f00) >> 8] << 8))


static void blink_cur(void) {
 cell_t cur;
 CURSOR_LOCK
 assert(vga_bufpos <= vga_bufend);
 if (vga_bufpos != vga_bufend) {
  cur = *BUF_CELL_CUR();
  cursor_inverted ^= 1;
  if (cursor_inverted) cur = INVERT(cur);
  *DEV_CELL_CUR() = cur;
 }
 CURSOR_UNLOCK
}
static void *cursor_blink_threadmain(void *__unused(closure)) {
 for (;;) { sleep(1); blink_cur(); }
 return NULL;
}
static void invert_all(void) {
 cell_t *iter,*end,cell;
 iter = vga_buf,end = vga_bufend;
 for (; iter != end; ++iter) {
  cell  = *iter;
  cell  = INVERT(cell);
  *iter = cell;
 }
}


static void TERM_CALL term_putc(struct term *__unused(term), char ch) {
 /* v This introducing lag is actually something that's currently intended. */
 //k_syslogf(KLOG_MSG,"%c",ch);
 BEGIN_MOVE_CUR()
 switch (ch) {
  case TERM_CR: CR(); goto end_moved;
  case TERM_LF:
   //printf(":AFTER LS: %d\n",vga_bufpos == vga_bufend);
   if (vga_bufpos == vga_bufend) goto scroll_one;
   else { LF(); goto end_moved; }
   break;
  case TERM_BACK: BACK(); goto end_moved; break;
  case TERM_TAB:  SET_CUR_X(align(GET_CUR_X(),TERM_TABSIZE)); break;
#ifdef TERM_BELL
  {
   struct timespec rem;
   struct timespec timeout = {0,10};
  case TERM_BELL: // Bell
   invert_all();
   nanosleep(&timeout,&rem);
   invert_all();
   break;
  }
#endif
  default: break;
 }
 if (vga_bufpos == vga_bufend) {
scroll_one:
  // Scroll at the end of the terminal
  memmove(vga_buf,vga_bufsecondline,
         (VGA_SIZE-VGA_WIDTH)*sizeof(cell_t));
  memcpy(vga_lastline,vga_spaceline,VGA_WIDTH*sizeof(cell_t));
  vga_bufpos = vga_lastline;
#if INVERT_CURSOR_AFTER_MOVE
  if (ch != '\n') {
   if (cursor_blink_enabled) {
    cursor_inverted = 1;
    *vga_bufpos++ = INVCHR(ch);
    BLIT();
    vga_bufpos[-1] = CHR(ch);
   } else {
    *vga_bufpos++ = CHR(ch);
    BLIT();
   }
  } else {
   BLIT();
  }
#else
  if (ch != '\n') *vga_bufpos++ = CHR(ch);
  BLIT();
#endif
 } else {
  *vga_bufpos = CHR(ch);
  BLIT_CUR();
  ++vga_bufpos;
end_moved:
  assert(vga_bufpos <= vga_bufend);
#if INVERT_CURSOR_AFTER_MOVE
  if (cursor_blink_enabled) {
   cursor_inverted = 1;
   if (vga_bufpos != vga_bufend) {
    BLIT_CUR_INV();
   }
  }
#endif
 }
 END_MOVE_CUR()
}

static uint8_t const vga_box_chars[] = {
 178,0,0,0,0,248,241,0,0,217,191,218,192,197,196,
 196,196,196,196,195,180,193,194,179,243,242,220
};

static void TERM_CALL term_putb(struct term *__unused(term), char ch) {
 assert(ch >= 'a' && ch <= 'z');
 ch = vga_box_chars[ch-'a'];
 /*if (ch)*/ term_putc(NULL,ch);
}

static void TERM_CALL
term_set_color(struct term *__unused(term),
               struct term_rgba fg,
               struct term_rgba bg) {
 vga_attrib = (vga_color(fg) | vga_color(bg) << 4) << 8;
}
static void TERM_CALL term_set_cursor(struct term *__unused(term), coord_t x, coord_t y) {
 if (x >= VGA_WIDTH)  x = *(__s32 *)&x < 0 ? 0 : VGA_WIDTH-1;
 if (y >= VGA_HEIGHT) y = *(__s32 *)&y < 0 ? 0 : VGA_HEIGHT-1;
 BEGIN_MOVE_CUR()
 SET_CUR(x,y);
#if INVERT_CURSOR_AFTER_MOVE
 if (cursor_blink_enabled) {
  cursor_inverted = 1;
  BLIT_CUR_INV();
 }
#endif
 END_MOVE_CUR()
}
static void TERM_CALL term_get_cursor(struct term *__unused(term), coord_t *x, coord_t *y) {
 *x = GET_CUR_X();
 *y = GET_CUR_Y();
}
static void TERM_CALL term_show_cursor(struct term *__unused(term), int cmd) {
 if (cmd == TERM_SHOWCURSOR_YES) {
  cursor_enable();
 } else {
  cursor_disable();
 }
}
static void TERM_CALL term_cls(struct term *__unused(term), int mode) {
 cell_t *begin,*end,*iter,filler = SPACE;
 switch (mode) {
  case TERM_CLS_BEFORE: begin = vga_buf; end = vga_bufpos; break;
  case TERM_CLS_AFTER : begin = vga_bufpos; end = vga_bufend; break;
  default             : begin = vga_buf; end = vga_bufend; break;
 }
 for (iter = begin; iter != end; ++iter) *iter = filler;
 BLIT_CNT(begin-vga_buf,end-begin);
}
static void TERM_CALL term_el(struct term *__unused(term), int mode) {
 cell_t *begin,*end,*iter,filler = SPACE;
 switch (mode) {
  case TERM_EL_BEFORE: begin = CUR_LINE(); end = vga_bufpos; break;
  case TERM_EL_AFTER : begin = vga_bufpos; end = CUR_LINE()+VGA_WIDTH; break;
  default            : begin = CUR_LINE(); end = begin+VGA_WIDTH; break;
 }
 for (iter = begin; iter != end; ++iter) *iter = filler;
 BLIT_CNT(begin-vga_buf,end-begin);
}
static void TERM_CALL term_scroll(struct term *__unused(term), offset_t offset) {
 cell_t filler = SPACE;
 cell_t *new_begin,*new_end;
 size_t copycells;
 if (!offset) return;
 if (offset >= VGA_HEIGHT || offset <= -VGA_HEIGHT) {
  term_cls(NULL,TERM_CLS_ALL);
  return;
 }
 if (offset > 0) {
  // Shift lines upwards
  copycells = (VGA_HEIGHT-offset)*VGA_WIDTH;
  memmove(vga_buf,vga_buf+offset*VGA_WIDTH,copycells*sizeof(cell_t));
  new_begin = vga_buf+copycells;
  new_end = vga_bufend;
 } else {
  // Shift lines downwards
  copycells = (VGA_HEIGHT+offset)*VGA_WIDTH;
  memmove(vga_buf-offset*VGA_WIDTH,vga_buf,copycells*sizeof(cell_t));
  new_end = (new_begin = vga_buf)+copycells;
 }
 for (; new_begin != new_end; ++new_begin)
  *new_begin = filler;
 BLIT();
}

static struct term_operations const term_ops = {
 &term_putc,
 &term_putb,
 &term_set_color,
 &term_set_cursor,
 &term_get_cursor,
 &term_show_cursor,
 &term_cls,
 &term_el,
 &term_scroll,
 NULL, // &term_set_title,
 NULL, // &term_set_cell_img,
 NULL, // &term_get_cell_size,
 NULL, // &term_output,
};

static struct winsize const winsize = {
 .ws_row    = VGA_HEIGHT,
 .ws_col    = VGA_WIDTH,
 .ws_xpixel = VGA_WIDTH,
 .ws_ypixel = VGA_HEIGHT,
};

static __noreturn void
usage(char const *name, int exitcode) {
 printf("Usage: %s EXEC-AFTER\n",name);
 _exit(exitcode);
}


static struct term *terminal;
static int amaster,aslave;

static void *relay_slave_out_threadmain(void *__unused(closure)) {
 ssize_t s; char buf[128];
 while ((s = read(amaster,buf,sizeof(buf))) >= 0) {
  term_outl(terminal,buf,(size_t)s);
 }
 term_outf(terminal,"[TERMINAL-VGA] Unexpected slave output read failure: %s\n",
           strerror(errno));
 return NULL;
}

// Parse and relay keyboard-style inputs from the terminal driver's STDIN
static void *relay_incoming_threadmain(void *__unused(closure)) {
 char text[8]; char *iter;
 struct kbevent event; ssize_t s;
 while ((s = read(STDIN_FILENO,&event,sizeof(event))) >= 0) {
  if (s < sizeof(struct kbevent)) { errno = EBUFSIZ; break; }
  if (KEY_ISDOWN(event.e_key)) {
   iter = text;
   switch (event.e_key) {
    case KEY_F1      : iter = strcpy(text,TERM_ESCAPE_S "OP"); break;
    case KEY_F2      : iter = strcpy(text,TERM_ESCAPE_S "OQ"); break;
    case KEY_F3      : iter = strcpy(text,TERM_ESCAPE_S "OR"); break;
    case KEY_F4      : iter = strcpy(text,TERM_ESCAPE_S "OS"); break;
    case KEY_F5      : iter = strcpy(text,TERM_ESCAPE_S "[15~"); break;
    case KEY_F6      : iter = strcpy(text,TERM_ESCAPE_S "[17~"); break;
    case KEY_F7      : iter = strcpy(text,TERM_ESCAPE_S "[18~"); break;
    case KEY_F8      : iter = strcpy(text,TERM_ESCAPE_S "[19~"); break;
    case KEY_F9      : iter = strcpy(text,TERM_ESCAPE_S "[20~"); break;
    case KEY_F10     : iter = strcpy(text,TERM_ESCAPE_S "[21~"); break;
    case KEY_F11     : iter = strcpy(text,TERM_ESCAPE_S "[23~"); break;
    case KEY_F12     : iter = strcpy(text,TERM_ESCAPE_S "[24~"); break;
    case KEY_UP      : iter = KBSTATE_ISCTRL(event.e_state) ? strcpy(text,TERM_ESCAPE_S "OA") : strcpy(text,TERM_ESCAPE_S "[A"); break;
    case KEY_DOWN    : iter = KBSTATE_ISCTRL(event.e_state) ? strcpy(text,TERM_ESCAPE_S "OB") : strcpy(text,TERM_ESCAPE_S "[B"); break;
    case KEY_RIGHT   : iter = KBSTATE_ISCTRL(event.e_state) ? strcpy(text,TERM_ESCAPE_S "OC") : strcpy(text,TERM_ESCAPE_S "[C"); break;
    case KEY_LEFT    : iter = KBSTATE_ISCTRL(event.e_state) ? strcpy(text,TERM_ESCAPE_S "OD") : strcpy(text,TERM_ESCAPE_S "[D"); break;
    case KEY_PAGEUP  : iter = strcpy(text,TERM_ESCAPE_S "[5~"); break;
    case KEY_PAGEDOWN: iter = strcpy(text,TERM_ESCAPE_S "[6~"); break;
    case KEY_HOME    : iter = strcpy(text,TERM_ESCAPE_S "OH"); break;
    case KEY_END     : iter = strcpy(text,TERM_ESCAPE_S "OF"); break;
    case KEY_DELETE  : iter = strcpy(text,TERM_ESCAPE_S "[3~"); break;
    case '\t':
     if (KBSTATE_ISSHIFT(event.e_state)) {
      *iter++ = TERM_ESCAPE;
      *iter++ = '[';
      *iter++ = 'Z';
      break;
     }
    default:
     if (KEY_ISUTF8(event.e_key)) {
      char ch = KEY_TOUTF8(event.e_key);
      if (!KBSTATE_ISALT(event.e_state) ||
          !KBSTATE_ISCTRL(event.e_state)) {
       if (KBSTATE_ISALT(event.e_state)) *iter++ = TERM_ESCAPE;
       if (KBSTATE_ISCTRL(event.e_state)) {
        // Create control character (e.g.: CTRL+C --> '\03')
        ch = toupper(ch)-'@';
       }
      }
      *iter++ = ch;
     }
     break;
   }
   if (iter != text) {
    //printf("[TERMINAL-VGA] Relay: %Iu %.*q\n",
    //        iter-text,(unsigned)(iter-text),text);
    if (write(amaster,text,(size_t)(iter-text)) == -1)
     perror("Relay failed");
   }
  }
 }
 term_outf(terminal,"[TERMINAL-VGA] Unexpected input relay failure: %s\n",
           strerror(errno));
 return NULL;
}



int main(int argc, char *argv[]) {
 int child_proc,result;
 if (argc < 2) { usage(argv[0],EXIT_FAILURE); }

 // Map the x86 VGA terminal.
 vga_dev = (cell_t *)mmapdev(0,VGA_SIZE*sizeof(cell_t),
                             PROT_READ|PROT_WRITE,
                             MAP_PRIVATE,VGA_ADDR);
 if (vga_dev == (cell_t *)(uintptr_t)-1) {
  perror("Failed to map x86 VGA terminal");
  _exit(EXIT_FAILURE);
 }
 //*(uintptr_t *)0 = 42;

 k_syslogf(KLOG_DEBUG,"Mapped terminal device from %p to %p\n",VGA_ADDR,vga_dev);
 vga_buf = (cell_t *)malloc((VGA_SIZE+VGA_WIDTH)*sizeof(cell_t));
 if (!vga_buf) {
  perror("Failed to allocate VGA buffer");
  _exit(EXIT_FAILURE);
 }
 vga_bufend = vga_buf+VGA_SIZE;
 vga_bufpos = vga_buf;
 vga_bufsecondline = vga_buf+VGA_WIDTH;
 vga_lastline = vga_buf+(VGA_SIZE-VGA_WIDTH);
 {
  cell_t *end = vga_bufend+VGA_WIDTH;
  cell_t *iter = vga_buf,filler = SPACE;
  for (; iter != end; ++iter) *iter = filler;
 }

 terminal = term_new(NULL,NULL,&term_ops,NULL);
 if (!terminal) {
  perror("Failed to create terminal");
  _exit(EXIT_FAILURE);
 }
 // The terminal driver runtime is loaded and operational!

 // Create the PTY device.
 if (openpty(&amaster,&aslave,NULL,NULL,&winsize) == -1) {
  perror("Failed to create PTY device");
  _exit(EXIT_FAILURE);
 }

#if 1
 if ((child_proc = task_fork()) == 0) {
  // === Child process
  // Redirect all standard files to the terminal
  dup2(aslave,STDIN_FILENO);
  dup2(aslave,STDOUT_FILENO);
  dup2(aslave,STDERR_FILENO);
  // Close all other descriptors
  kfd_closeall(3,INT_MAX);
  // Exec the given process
  execl(argv[1],argv[1],NULL);
  perror("Failed to exec given process");
  _exit(errno);
 } else if (child_proc == -1)
#endif
 {
  perror("Failed to fork child process");
  _exit(EXIT_FAILURE);
 }

 // Close the slave end of the terminal driver
 close(aslave);

 // Spawn helper threads
 cursor_blink_thread = task_newthread(&cursor_blink_threadmain,NULL,TASK_NEWTHREAD_DEFAULT);
 relay_slave_out_thread = task_newthread(&relay_slave_out_threadmain,NULL,TASK_NEWTHREAD_DEFAULT);
 relay_incoming_thread = task_newthread(&relay_incoming_threadmain,NULL,TASK_NEWTHREAD_DEFAULT);

 k_syslogf(KLOG_DEBUG,"Updating screen for the first time\n");
 // Do an initial blit to clear any leftovers originating from the kernel
 BLIT();

 {
  // Join the slave process
  uintptr_t child_exit;
  k_syslogf(KLOG_DEBUG,"Begin joining child process\n");
  if (proc_join(child_proc,&child_exit) == -1) {
   perror("Failed to join child process");
   result = EXIT_FAILURE;
  } else {
   result = (int)child_exit;
  }
 }

 // Cleanup...
 close(child_proc);
 task_terminate(cursor_blink_thread,NULL);
 task_terminate(relay_slave_out_thread,NULL);
 task_terminate(relay_incoming_thread,NULL);
 close(cursor_blink_thread);
 close(relay_slave_out_thread);
 close(relay_incoming_thread);
 term_delete(terminal);
 free(vga_buf);
 munmap(vga_dev,VGA_SIZE*sizeof(cell_t));

 return result;
}


#endif /* !__TERMINAL_VGA_C__ */
