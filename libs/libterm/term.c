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
#ifndef __TERM_C__
#define __TERM_C__ 1

#include <kos/compiler.h>
#include <term/term.h>
#include <malloc.h>
#include <errno.h>
#include <format-printer.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <kos/atomic.h>
#include <kos/task.h>

#include "xterm_256.h"

__DECL_BEGIN

#define C(r,g,b) TERM_RGBA_INIT(r,g,b,0xff)
#define UNDEF   C(255,255,255)

__public struct term_palette const
term_palettes[TERM_PALETTES] = {
 {"Standard VGA colors",{
  C(0,0,0),C(170,0,0),C(0,170,0),C(170,85,0),C(0,0,170),C(170,0,170),C(0,170,170),C(170,170,170),
  C(85,85,85),C(255,85,85),C(85,255,85),C(255,255,85),C(85,85,255),C(255,85,255),C(85,255,255),C(255,255,255),
 }},
 {"Windows XP CMD",{
  C(0,0,0),C(128,0,0),C(0,128,0),C(128,128,0),C(0,0,128),C(128,0,128),C(0,128,128),C(192,192,192),
  C(128,128,128),C(255,0,0),C(0,255,0),C(255,255,0),C(0,0,255),C(255,0,255),C(0,255,255),C(255,255,255),
 }},
 {"Terminal.app",{
  C(0,0,0),C(194,54,33),C(37,188,36),C(173,173,39),C(73,46,225),C(211,56,211),C(51,187,200),C(203,204,205),
  C(129,131,131),C(252,57,31),C(49,231,34),C(234,236,35),C(88,51,255),C(249,53,248),C(20,240,240),C(233,235,235),
 }},
 {"PuTTY",{
  C(0,0,0),C(187,0,0),C(0,187,0),C(187,187,0),C(0,0,187),C(187,0,187),C(0,187,187),C(187,187,187),
  C(85,85,85),C(255,85,85),C(85,255,85),C(255,255,85),C(85,85,255),C(255,85,255),C(85,255,255),C(255,255,255),
 }},
 {"mIRC",{
  C(0,0,0),C(127,0,0),C(0,147,0),C(252,127,0),C(0,0,127),C(156,0,156),C(0,147,147),C(210,210,210),
  C(127,127,127),C(255,0,0),C(0,252,0),C(255,255,0),C(0,0,252),C(255,0,255),C(0,255,255),C(255,255,255),
 }},
 {"xterm",{
  C(0,0,0),C(205,0,0),C(0,205,0),C(205,205,0),C(0,0,238),C(205,0,205),C(0,205,205),C(229,229,229),
  C(127,127,127),C(255,0,0),C(0,255,0),C(255,255,0),C(92,92,255),C(255,0,255),C(0,255,255),C(255,255,255),
 }},
 {"CSS/HTML",{
  C(0,0,0),C(255,0,0),C(0,255,0),C(255,255,0),C(0,0,255),C(255,0,255),C(0,255,255),C(255,255,255),
  UNDEF,UNDEF,C(144,238,144),C(255,255,224),C(173,216,230),UNDEF,C(224,255,255),UNDEF,
 }},
 {"X",{
  C(0,0,0),C(255,0,0),C(0,128,0),C(255,255,0),C(0,0,255),C(255,0,255),C(0,255,255),C(255,255,255),
  UNDEF,UNDEF,C(144,238,144),C(225,255,224),C(173,216,230),UNDEF,C(224,255,255),UNDEF,
 }},
};
#include <math.h>

#define COORD_MAX  UINT32_MAX

#define SELF                   self
#define PUTC(ch)             (*SELF->tr_ops.to_putc)(SELF,ch)
#define PUTB(ch)             (*SELF->tr_ops.to_putb)(SELF,ch)
#define SET_COLOR(fg,bg)     (*SELF->tr_ops.to_set_color)(SELF,fg,bg)
#define SET_CURSOR(x,y)      (*SELF->tr_ops.to_set_cursor)(SELF,x,y)
#define GET_CURSOR(x,y)      (*SELF->tr_ops.to_get_cursor)(SELF,&(x),&(y))
#define SHOW_CURSOR(cmd)     (*SELF->tr_ops.to_show_cursor)(SELF,cmd)
#define CLS(mode)            (*SELF->tr_ops.to_cls)(SELF,mode)
#define EL(mode)             (*SELF->tr_ops.to_el)(SELF,mode)
#define SCROLL(offset)       (*SELF->tr_ops.to_scroll)(SELF,offset)
#define SET_FONT_SIZE(scale) (*SELF->tr_ops.to_set_font_size)(SELF,scale)
#define SET_TITLE(text)      (*SELF->tr_ops.to_set_title)(SELF,text)
#define PUTIMG(image)        (*SELF->tr_ops.to_putimg)(SELF,image)
#define GET_CELL_SIZE(x,y)   (*SELF->tr_ops.to_get_cell_size)(SELF,&(x),&(y))
#define OUTPUT(text)         (*SELF->tr_ops.to_output)(SELF,text)


// Some of these stubs can be implemented using different operators...
static void TERM_CALL stub_putb(struct term *self, char ch) {
 switch (ch) {
  case ANSIBOX_VLINE:    PUTC('|'); break;
  case ANSIBOX_HLINE:    PUTC('-'); break;
  case ANSIBOX_ULCORNER:
  case ANSIBOX_URCORNER:
  case ANSIBOX_LLCORNER:
  case ANSIBOX_LRCORNER: PUTC('+'); break;
  default:               PUTC('\0'); break;
 }
}
static void TERM_CALL stub_set_color(struct term *__unused(self), struct term_rgba __unused(fg), struct term_rgba __unused(bg)) {}
static void TERM_CALL stub_set_cursor(struct term *__unused(self), coord_t __unused(x), coord_t __unused(y)) {}
static void TERM_CALL stub_get_cursor(struct term *__unused(self), coord_t *x, coord_t *y) { *x = *y = 0; }
static void TERM_CALL stub_show_cursor(struct term *__unused(self), int __unused(cmd)) {}
static void TERM_CALL stub_cls(struct term *self, int mode) {
 coord_t sx,sy,x,y; size_t i;
 switch (mode) {
  case TERM_CLS_AFTER:
   GET_CURSOR(x,y);
   SET_CURSOR(COORD_MAX,COORD_MAX);
   GET_CURSOR(sx,sy);
   SET_CURSOR(x,y);
   i = ((sy-y)*sx)-x;
   break;
  case TERM_CLS_BEFORE:
   GET_CURSOR(x,y);
   SET_CURSOR(COORD_MAX,COORD_MAX);
   GET_CURSOR(sx,sy);
   SET_CURSOR(0,0);
   i = (y*sx)+x;
   break;
  default:
   SET_CURSOR(COORD_MAX,COORD_MAX);
   GET_CURSOR(sx,sy);
   SET_CURSOR(0,0);
   i = sx*sy;
   break;
 }
 while (i--) PUTC(' ');
}
static void TERM_CALL stub_el(struct term *self, int mode) {
 coord_t sx,x,y; size_t i;
 GET_CURSOR(x,y);
 SET_CURSOR(COORD_MAX,y);
 GET_CURSOR(sx,y);
 switch (mode) {
  case TERM_EL_AFTER:  SET_CURSOR(x,y); i = sx-x; break;
  case TERM_EL_BEFORE: SET_CURSOR(0,y); i = x; break;
  default:             SET_CURSOR(0,y); i = sx; break;
 }
 while (i--) PUTC(' ');
}
static void TERM_CALL stub_scroll(struct term *self, offset_t offset) {
 if (offset <= 0) return;
 while (offset--) {
  SET_CURSOR(0,COORD_MAX);
  PUTC('\n');
 }
}
static void TERM_CALL stub_set_title(struct term *__unused(self), char *__unused(text)) {}
static void TERM_CALL stub_putimg(struct term *self, struct term_rgba image[]) { (void)image; PUTC('?'); }
static void TERM_CALL stub_get_cell_size(struct term *__unused(self), size_t *x, size_t *y) { *x = *y = 8; }
static void TERM_CALL stub_output(struct term *__unused(self), char *__unused(text)) {}

#define UPDATE_FG_PALETTE() \
 (SELF->tr_fg = SELF->tr_palette_sel->tp_colors[SELF->tr_palette_idx & 0x0f])
#define UPDATE_BG_PALETTE() \
 (SELF->tr_bg = SELF->tr_palette_sel->tp_colors[(SELF->tr_palette_idx & 0xf0) >> 4])

__local void term_selpalette(struct term *self, uint8_t idx) {
 if (self->tr_palette_idx == idx) return;
 self->tr_palette_idx = idx;
 UPDATE_FG_PALETTE();
 UPDATE_BG_PALETTE();
 SET_COLOR(self->tr_fg,self->tr_bg);
}

__local void
__term_init(struct term *self, struct term_palette const *palette,
            struct term_operations const *ops) {
 memcpy(&self->tr_ops,ops,sizeof(struct term_operations));
 if (!self->tr_ops.to_putb)          self->tr_ops.to_putb          = &stub_putb;
 if (!self->tr_ops.to_set_color)     self->tr_ops.to_set_color     = &stub_set_color;
 if (!self->tr_ops.to_set_cursor)    self->tr_ops.to_set_cursor    = &stub_set_cursor;
 if (!self->tr_ops.to_get_cursor)    self->tr_ops.to_get_cursor    = &stub_get_cursor;
 if (!self->tr_ops.to_show_cursor)   self->tr_ops.to_show_cursor   = &stub_show_cursor;
 if (!self->tr_ops.to_cls)           self->tr_ops.to_cls           = &stub_cls;
 if (!self->tr_ops.to_el)            self->tr_ops.to_el            = &stub_el;
 if (!self->tr_ops.to_scroll)        self->tr_ops.to_scroll        = &stub_scroll;
 if (!self->tr_ops.to_set_title)     self->tr_ops.to_set_title     = &stub_set_title;
 if (!self->tr_ops.to_putimg)        self->tr_ops.to_putimg        = &stub_putimg;
 if (!self->tr_ops.to_get_cell_size) self->tr_ops.to_get_cell_size = &stub_get_cell_size;
 if (!self->tr_ops.to_output)        self->tr_ops.to_output        = &stub_output;
 self->tr_palette_sel = palette ? palette : &term_palettes[0];
 self->tr_escape = 0;
 self->tr_attrib = 0;
 self->tr_mouseon = TERM_MOUSEON_NO;
 self->tr_image = NULL;
 self->tr_lock = 0;
 term_selpalette(self,TERM_PALETTE(TERM_DEFAULT_FG,TERM_DEFAULT_BG));
}

__public struct term *
term_new(struct term *buf, struct term_palette const *palette,
         struct term_operations const *ops, void *closure) {
 struct term *result;
 size_t ansi_size; char *ansi_buf;
 size_t ansi_arga; char **ansi_argv;
 if (!ops || !ops->to_putc) {
  __set_errno(EINVAL);
  return NULL;
 }
 ansi_size = 128;
 ansi_arga = 4;
 for (;;) {
  ansi_buf = (char *)malloc((ansi_size+1)*sizeof(char));
  if __likely(ansi_buf) break;
  ansi_size /= 2;
  if __unlikely(!ansi_size) return NULL;
 }
 for (;;) {
  ansi_argv = (char **)malloc(ansi_arga*sizeof(char *));
  if __likely(ansi_argv) break;
  ansi_arga /= 2;
  if __unlikely(!ansi_arga) goto err_ansibuf;
 }
 if (buf) {
  result          = buf;
  result->tr_flags = TERMINAL_FLAG_NONE;
 } else {
  if ((result = omalloc(struct term)) == NULL) goto err_ansiargv;
  result->tr_flags = TERMINAL_FLAG_FREE;
 }
 result->tr_user = closure;
 result->tr_buffer = ansi_buf;
 result->tr_bufend = ansi_buf+ansi_size;
 *result->tr_bufend = '\0';
 result->tr_arga = ansi_arga;
 result->tr_argv = ansi_argv;
 ansi_argv[0] = ansi_buf;
 __term_init(result,palette,ops);
 return result;
err_ansiargv: free(ansi_argv);
err_ansibuf: free(ansi_buf);
 return NULL;
}

__public int term_delete(struct term *self) {
 if __unlikely(!self) { __set_errno(EINVAL); return -1; }
 free(self->tr_image);
 free(self->tr_buffer);
 free(self->tr_argv);
 if (self->tr_flags&TERMINAL_FLAG_FREE) free(self);
 return 0;
}



__local int resize_buffer(struct term *self, size_t newsize) {
 char *oldbuffer,*newbuffer;
 char **arg_iter,**arg_end;
 assert(newsize);
 assert(self->tr_escape != 0);
 assert(self->tr_arga != 0);
 assert(self->tr_argc <= self->tr_arga);
 assert(self->tr_bufend != self->tr_buffer);
 oldbuffer = self->tr_buffer;
 newbuffer = (char *)realloc(oldbuffer,(newsize+1)*sizeof(char));
 if __unlikely(!newbuffer) return 0;
 arg_end = (arg_iter = self->tr_argv)+self->tr_argc;
 for (; arg_iter != arg_end; ++arg_iter) {
  assert(*arg_iter >= oldbuffer && *arg_iter < self->tr_bufend);
  *arg_iter = newbuffer+(*arg_iter-oldbuffer);
 }
 self->tr_argv[0] = newbuffer;
 self->tr_bufpos = newbuffer+(self->tr_bufpos-oldbuffer);
 self->tr_buffer = newbuffer;
 self->tr_bufend = newbuffer+newsize;
 newbuffer[newsize] = '\0';
 return 1;
}

static char const *const escape_preface[] = {
 "",
 ANSI_ESCAPE_S,
 ANSI_ESCAPE_S ANSI_BRACKET_S,
 ANSI_ESCAPE_S ANSI_BRACKET_RIGHT_S,
 ANSI_ESCAPE_S ANSI_OPEN_PAREN_S,
 ANSI_ESCAPE_S "T",
};

__local void _term_outc(struct term *self, char ch) {
 char **iter,**end;
 assert(self);
 switch (self->tr_escape) {
  case 0:
   if (ch == ANSI_ESCAPE) {
enter_escape_mode:
    /* Enable escape mode. */
    self->tr_escape = 1;
    self->tr_argc   = 0;
    self->tr_bufpos = self->tr_buffer;
    return;
   }
   break;
  case 1:
   switch (ch) {
    case ANSI_BRACKET:       self->tr_escape = 2; return;
    case ANSI_BRACKET_RIGHT: self->tr_escape = 3; return;
    case ANSI_OPEN_PAREN:    self->tr_escape = 4; return;
    case 'T':                self->tr_escape = 5; return;
    default: /* Not actually escaping... */ goto dump_buffer;
   }
   break;

  {
  case 2: /* "ESC[" */
#define ARGC  (SELF->tr_argc)
#define ARGV  (SELF->tr_argv)
   if (ch < ANSI_LOW || ch > ANSI_HIGH) goto add_to_buffer;
   /* End escape sequence (Terminate current argument) */
   assert(self->tr_bufpos <= self->tr_bufend);
   *self->tr_bufpos = '\0';
   /* Let's see what we got! */
   switch (ch) {
    /* Save cursor position */
    case ANSI_SCP: GET_CURSOR(self->tr_savex,self->tr_savey); break;
    /* Restore cursor position */
    case ANSI_RCP: SET_CURSOR(self->tr_savex,self->tr_savey); break;

    {
     uint8_t new_colors;
     struct term_rgba new_fg,new_bg;
     int real_color_mode;
#define REAL_FG 0x1
#define REAL_BG 0x2
     /* Set Graphics Rendition */
    case ANSI_SGR:
     real_color_mode = 0;
     new_colors = self->tr_palette_idx;
#define SET_TRUE_COLOR_MODE_FG()   { real_color_mode |= REAL_FG; }
#define SET_TRUE_COLOR_MODE_BG()   { real_color_mode |= REAL_BG; }
#define SET_TRUE_COLOR_MODE_FGBG() { real_color_mode |= REAL_FG|REAL_BG; }
     if (!ARGC) {
      new_colors = TERM_PALETTE(TERM_DEFAULT_FG,TERM_DEFAULT_BG);
      self->tr_attrib = 0;
     } else {
      end = (iter = ARGV)+ARGC;
      do {
       int arg = atoi(*iter++);
       switch (arg) {
        case 49: /* Reset Background */
         new_colors = (new_colors & 0x0f)|(TERM_DEFAULT_BG << 4);
         break;
        case 39: /* Reset Foreground */
         new_colors = (new_colors & 0xf0)|(TERM_DEFAULT_FG);
         break;
        case 9: self->tr_attrib |= ANSI_CROSS; break;
        case 7:
         new_colors = ((new_colors & 0x0f) << 4)|
                      ((new_colors & 0xf0) >> 4);
         break;
        case 6:
         /* proprietary RGBA color support */
         if ((end-iter) >= 4) {
          struct term_rgba color;
          color.r = atoi(iter[0]);
          color.g = atoi(iter[1]);
          color.b = atoi(iter[2]);
          color.a = atoi(iter[3]);
          if (iter != ARGV+1 && atoi(iter[-2]) == 48) {
           /* True color background */
           SET_TRUE_COLOR_MODE_BG();
           new_bg = color;
          } else {
           SET_TRUE_COLOR_MODE_FG();
           new_fg = color;
          }
          iter += 4;
         }
         break;
        {
         int color_index;
        case 5:
         /* X-term 256 colors. */
         if (iter == end) break;
         color_index = atoi(*iter);
              if (color_index < 0) color_index = 0;
         else if (color_index > 255) color_index = 255;
         if (iter != ARGV+1 && atoi(iter[-2]) == 48) {
          /* Background */
          SET_TRUE_COLOR_MODE_BG();
          new_bg = xterm_256[color_index];
         } else {
          /* Foreground */
          SET_TRUE_COLOR_MODE_FG();
          new_fg = xterm_256[color_index];
         }
         ++iter;
         break;
        }
        case 4: /* Enable Underline */
         self->tr_attrib |= ANSI_UNDERLINE;
         break;
        case 3: /* Enable Italic. */
         self->tr_attrib |= ANSI_ITALIC;
         break;
        case 2:
         /* Konsole RGB color support */
         if ((end-iter) >= 3) {
          struct term_rgba color;
          color.r = atoi(iter[0]);
          color.g = atoi(iter[1]);
          color.b = atoi(iter[2]);
          color.a = 0xff;
          if (iter != ARGV+1 && atoi(iter[-2]) == 48) {
           /* True color background */
           SET_TRUE_COLOR_MODE_BG();
           new_bg = color;
          } else {
           SET_TRUE_COLOR_MODE_FG();
           new_fg = color;
          }
          iter += 3;
         }
         break;
        case 1:
         /* Brighten foreground output color. */
         new_colors |= 0x8;
         break;
        case 0:
         /* Reset colors and attributes. */
         new_colors = TERM_PALETTE(TERM_DEFAULT_FG,TERM_DEFAULT_BG);
         real_color_mode = 0;
         self->tr_attrib = 0;
         break;
        default:
         /* Bright background */
         if (arg >= 100 && arg <= 107) {
          new_colors = (new_colors & 0x0f)|(((arg-100)|0x8) << 4);
          break;
         }
         /* Bright foreground */
         if (arg >= 90 && arg <= 97) {
          new_colors = (new_colors & 0xf0)|(arg-90)|0x8;
          break;
         }
         if (arg >= 40 && arg <= 47) {
          /* Normal background */
          new_colors = (new_colors & 0x0f)|((arg-40) << 4);
          break;
         }
         if (arg >= 30 && arg <= 37) {
          /* Normal Foreground */
          new_colors = (new_colors & 0xf0)|(arg-30);
          break;
         }
         break;
       }
      } while (iter != end);
     }
     /* Apply new colors/attributes. */
     self->tr_palette_idx = new_colors;
     if (!(real_color_mode&REAL_FG)) new_fg = self->tr_palette_sel->tp_colors[new_colors & 0x0f];
     if (!(real_color_mode&REAL_BG)) new_bg = self->tr_palette_sel->tp_colors[(new_colors & 0xf0) >> 4];
     if (new_fg.color != self->tr_fg.color ||
         new_bg.color != self->tr_bg.color) {
      self->tr_fg = new_fg;
      self->tr_bg = new_bg;
      goto update_colors;
     }
     break;
    }

    case ANSI_SHOW:
     if (ARGC && ARGV[0][0] == '?') {
      switch (atoi(ARGV[0]+1)) {
       case 1049: CLS(TERM_CLS_ALL); SET_CURSOR(0,0); break;
       case 1000: self->tr_mouseon = TERM_MOUSEON_YES; break;
       case 1002: self->tr_mouseon = TERM_MOUSEON_WITHMOTION; break;
       case 25:   SHOW_CURSOR(TERM_SHOWCURSOR_YES); break;
       default: break;
      }
     }
     break;

    case ANSI_HIDE:
     if (ARGC && ARGV[0][0] == '?') {
      switch (atoi(ARGV[0]+1)) {
       case 1049: /* TODO */ break;
       case 1000:
       case 1002: self->tr_mouseon = TERM_MOUSEON_NO; break;
       case 25:   SHOW_CURSOR(TERM_SHOWCURSOR_NO); break;
       default: break;
      }
     }
     break;

    {
     /* Move the cursor */
     coord_t x,y;
    case ANSI_CUF:
     GET_CURSOR(x,y);
     SET_CURSOR(x+(ARGC ? atoi(ARGV[0]) : 1),y);
     break;
    case ANSI_CUB:
     GET_CURSOR(x,y);
     SET_CURSOR(x-(ARGC ? atoi(ARGV[0]) : 1),y);
     break;
    case ANSI_CUD:
     GET_CURSOR(x,y);
     SET_CURSOR(x,y+(ARGC ? atoi(ARGV[0]) : 1));
     break;
    case ANSI_CUU:
     GET_CURSOR(x,y);
     SET_CURSOR(x,y-(ARGC ? atoi(ARGV[0]) : 1));
     break;
    case ANSI_CHA:
     GET_CURSOR(x,y);
     SET_CURSOR(ARGC ? (coord_t)(atoi(ARGV[0])-1) : 0,y);
     break;
    case 'd':
     GET_CURSOR(x,y);
     SET_CURSOR(x,ARGC ? (coord_t)(atoi(ARGV[0])-1) : 0);
     break;
    case ANSI_CUP:
     if (ARGC >= 2) {
      SET_CURSOR((coord_t)(atoi(ARGV[0])-1),
                 (coord_t)(atoi(ARGV[1])-1));
     } else SET_CURSOR(0,0);
     break;
    }
    case ANSI_ED: /* Erase everything. */
     CLS(ARGC ? atoi(ARGV[0]) : 0);
     break;
    case ANSI_EL: /* Erase line. */
     EL(ARGC ? atoi(ARGV[0]) : 0);
     break;
    {
     char buf[32];
     coord_t x,y;
    case ANSI_DSR:
     GET_CURSOR(x,y);
     sprintf(buf,ANSI_ESCAPE_S "[%d;%dR",y+1,x+1);
     OUTPUT(buf);
     break;
    }
    /* Scroll Up/Down */
    case ANSI_SU: SCROLL(ARGC ?  atoi(ARGV[0]) :  1); break;
    case ANSI_SD: SCROLL(ARGC ? -atoi(ARGV[0]) : -1); break;

    {
     int n;
    case 'X': /* Repeat space-characters... */
     n = ARGC ? atoi(ARGV[0]) : 1;
     while (n--) PUTC(' ');
     break;
    }


    default: goto dump_buffer;
   }
   goto end_escape;
  } break;

  {
  case 3: /* "ESC]" */
   end = (iter = ARGV)+ARGC;
   while (iter != end) {
    int arg = atoi(*iter++);
    if (arg == 1 && iter != end) {
     /* Set title */
     SET_TITLE(*iter);
     ++iter;
    }
   }
   goto end_escape;
  }

  {
  case 4: /* "ESC(" */
   if (ch == '0') {
    /* Enable BOX mode. */
    self->tr_escape = 10;
    self->tr_attrib |= ANSI_BOX;
    return;
   } else if (ch == 'B') {
    /* Disable BOX mode. */
    self->tr_attrib &= ~(ANSI_BOX);
    goto end_escape;
   }
   break;
  }

  {
   size_t x,y;
   char buf[32];
  case 5: /* "ESCT" */
   if (ch == 'q') {
    GET_CELL_SIZE(x,y);
    sprintf(buf,ANSI_ESCAPE_S "T%Iu;%Iuq",x,y);
    OUTPUT(buf);
   } else if (ch == 's') {
    if (!self->tr_image) {
     size_t image_size; GET_CELL_SIZE(x,y);
     image_size = x*y*sizeof(struct term_rgba);
     self->tr_image = (struct term_rgba *)malloc(image_size);
     self->tr_imgnd = self->tr_imabg ? self->tr_imabg+image_size : NULL;
    }
    if (!self->tr_imabg) goto dump_buffer;
    self->tr_imgps = self->tr_imabg;
    memset(self->tr_imabg,0,
          (self->tr_imgnd-self->tr_imabg));
    self->tr_escape = 6;
    return;
   } else {
    goto dump_buffer;
   }
   goto end_escape;
  }

  case 6: /* Draw RGBA cell image. */
   assert(self->tr_imgps);
   *self->tr_imgps++ = (uint8_t)ch;
   if (self->tr_imgps == self->tr_imgnd) {
    PUTIMG(self->tr_image);
    goto end_escape;
   }
   return;

  case 10: /* BOX mode. */
   if (ch == ANSI_ESCAPE) goto enter_escape_mode;
   if (ch >= 'a' && ch <= 'z') { PUTB(ch); return; }
   break;

  default: goto dump_buffer;
 }
 PUTC(ch);
 return;
update_colors:
 SET_COLOR(self->tr_fg,self->tr_bg);
end_escape:
 self->tr_escape = (self->tr_attrib&ANSI_BOX) ? 10 : 0;
 return;
add_to_buffer:
 if __unlikely(self->tr_bufpos == self->tr_bufend) {
  size_t new_size = (self->tr_bufend-self->tr_buffer)*2;
  assert(new_size);
  if (!resize_buffer(self,new_size)) goto dump_buffer;
 }
 // Already go ahead and split arguments
 assert(self->tr_argv[0] == self->tr_buffer);
 if (!self->tr_argc) self->tr_argc = 1; /*< Non-empty arg-text means at least one argument! */
 if (ch == ';') {
  *self->tr_bufpos++ = '\0'; /* Terminate current argument. */
  if (self->tr_argc == self->tr_arga) { /* Allocate more arguments. */
   char **newargv; size_t new_arga = self->tr_arga*2;
   assert(new_arga);
   newargv = (char **)realloc(self->tr_argv,new_arga*sizeof(char *));
   if (!newargv) goto dump_buffer;
   self->tr_arga = new_arga;
   self->tr_argv = newargv;
  }
  /* The next argument starts after this '\0'. */
  self->tr_argv[self->tr_argc++] = self->tr_bufpos;
 } else {
  *self->tr_bufpos++ = ch;
 }
 return;
dump_buffer:
 if (self->tr_escape != 0 && self->tr_escape != 10) {
  char *buf_iter,*buf_end;
  char const *preface;
  /* Repeat the original preface. */
  preface = escape_preface[self->tr_escape];
  for (; *preface; ++preface) PUTC(*preface);
  self->tr_escape = (self->tr_attrib&ANSI_BOX) ? 10 : 0;
  /* Dump the escape buffer. */
  buf_iter = self->tr_buffer,buf_end = self->tr_bufpos;
  for (; buf_iter != buf_end; ++buf_iter) PUTC(*buf_iter);
  /* Finally, print the character that caused the dump. */
  PUTC(ch);
 }
}

__public void term_outc(struct term *self, char ch) {
 assert(self);
 while (!katomic_cmpxch(self->tr_lock,0,1)) ktask_yield();
 _term_outc(self,ch);
 self->tr_lock = 0;
}



__public void term_outn(struct term *self, char const *p, size_t max_chars) {
 char const *iter,*end;
 assert(self);
 end = (iter = (char const *)p)+max_chars;
 for (; iter != end && *iter; ++iter) term_outc(self,*iter);
}
__public void term_outl(struct term *self, void const *p, size_t len) {
 char const *iter,*end;
 assert(self);
 assert(!len || p);
 end = (iter = (char const *)p)+len;
 for (; iter != end; ++iter) term_outc(self,*iter);
}
__public void term_outs(struct term *self, char const *s) {
 char ch; assert(self);
 if (s) while ((ch = *s++) != '\0') {
  term_outc(self,ch);
 }
}


static int term_printer(char const *__restrict data,
                        size_t maxchars,
                        struct term *__restrict terminal) {
 term_outn(terminal,data,maxchars);
 return 0;
}

__public void term_voutf(struct term *self, char const *fmt, va_list args) {
 format_vprintf((pformatprinter)&term_printer,self,fmt,args);
}
__public void term_outf(struct term *self, char const *fmt, ...) {
 va_list args;
 va_start(args,fmt);
 term_voutf(self,fmt,args);
 va_end(args);
}



__DECL_END

#endif /* !__TERM_C__ */
