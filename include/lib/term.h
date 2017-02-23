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
#ifndef __LIB_TERM_H__
#define __LIB_TERM_H__ 1

#include <kos/compiler.h>
#include <endian.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <uchar.h>

__DECL_BEGIN

// Compile with -lterm


// NOTE: Although this source doesn't include any license, I thought
//       I should mention that this ANSI terminal emulator is inspired
//       by this: http://dakko.us/~k/ansi.c.html

__COMPILER_PACK_PUSH(1)
struct __packed term_rgba {union __packed {
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TERM_RGBA_INIT(r,g,b,a) {{{a,b,g,r}}}
#define TERM_RGBA_CHANNEL_R 3
#define TERM_RGBA_CHANNEL_G 2
#define TERM_RGBA_CHANNEL_B 1
#define TERM_RGBA_CHANNEL_A 0
 struct __packed { uint8_t a,b,g,r; };
 struct __packed { int8_t sa,sb,sg,sr; };
#else
#define TERM_RGBA_INIT(r,g,b,a) {{{r,g,b,a}}}
#define TERM_RGBA_CHANNEL_R 0
#define TERM_RGBA_CHANNEL_G 1
#define TERM_RGBA_CHANNEL_B 2
#define TERM_RGBA_CHANNEL_A 3
 struct __packed { uint8_t r,g,b,a; };
 struct __packed { int8_t sr,sg,sb,sa; };
#endif
                   uint32_t color;
                   uint8_t channels[4];
};};
__COMPILER_PACK_POP

typedef __u32 coord_t;
typedef __s32 offset_t;

struct term;
#if defined(__i386__)
#define TERM_CALL __fastcall
#else
#define TERM_CALL /* nothing */
#endif



#define ANSI_COLORS       16
#define ANSI_CL_BLACK     0
#define ANSI_CL_RED       1
#define ANSI_CL_GREEN     2
#define ANSI_CL_YELLOW    3
#define ANSI_CL_BLUE      4
#define ANSI_CL_MAGENTA   5
#define ANSI_CL_CYAN      6
#define ANSI_CL_WHITE     7
#define ANSI_IFSTRONG     0x8 /*< Intensify a given ANSI color. */
#define ANSI_ISSTRONG(x) ((x)&ANSI_IFSTRONG)
#define ANSI_TOSTRONG(x) ((x)|ANSI_IFSTRONG)

struct term_palette {
 char const      *tp_name;                /*< [0..1] Palette name (If any). */
 struct term_rgba tp_colors[ANSI_COLORS]; /*< ANSI Colors mapped to palette colors. */
};
#define TERM_PALETTES  8
extern struct term_palette const term_palettes[TERM_PALETTES];
// NOTE: Supported palettes can be found here:
//    >> https://en.wikipedia.org/wiki/ANSI_escape_code#Colors



struct term_operations {
    void    (TERM_CALL *to_putc)(struct term *term, char ch); /* [MANDATORY] Write a single character/byte. */
    void    (TERM_CALL *to_putb)(struct term *term, char ch); /* Write a a-z-style box character (NOTE: 'ch' is a lower-case ascii character). */
    void    (TERM_CALL *to_set_color)(struct term *term, struct term_rgba fg, struct term_rgba bg); /* Set the current text color. */
    void    (TERM_CALL *to_set_cursor)(struct term *term, coord_t x, coord_t y); /* Set the position of the cursor. */
    void    (TERM_CALL *to_get_cursor)(struct term *term, coord_t *x, coord_t *y); /* Returns the position of the cursor. */
#define TERM_SHOWCURSOR_NO  0
#define TERM_SHOWCURSOR_YES 1
    void    (TERM_CALL *to_show_cursor)(struct term *term, int cmd); /* Show/Hide the cursor. */
#define TERM_CLS_AFTER  0 /*< Clear everything after the cursor (including the cursor itself). */
#define TERM_CLS_BEFORE 1 /*< Clear everything before the cursor (excluding the cursor itself). */
#define TERM_CLS_ALL    2 /*< Clear everything. */
    void    (TERM_CALL *to_cls)(struct term *term, int mode);
#define TERM_EL_AFTER   0 /*< Erase the line after the cursor (including the cursor itself). */
#define TERM_EL_BEFORE  1 /*< Erase the line before the cursor (excluding the cursor itself). */
#define TERM_EL_ALL     2 /*< Erase the line. */
    void    (TERM_CALL *to_el)(struct term *term, int mode);
    /* Shift terminal lines by offset, where a negative value shifts
       lines downwards, and a positive value shifts them up.
       e.g.: When the end of the terminal is reached, the driver would call '(*to_scroll)(...,1);' */
    void    (TERM_CALL *to_scroll)(struct term *term, offset_t offset);
    void    (TERM_CALL *to_set_title)(struct term *term, char *text); /*< Set the title of the terminal. */
    /* Direct access to pixels within the current cell (The image array has to_get_cell_size:x*y elements).
       NOTE: Just like 'to_putc', the cursor is automatically advanced. */
    void    (TERM_CALL *to_putimg)(struct term *term, struct term_rgba image[]);
    void    (TERM_CALL *to_get_cell_size)(struct term *term, size_t *x, size_t *y); /* Returns the size of a cell in pixels. */
    void    (TERM_CALL *to_output)(struct term *term, char *text); /*< Output 'text' to the slave process ('write(amaster,text,strlen(text))'; amaster from <pty.h>:openpty) */
};

struct term {
 void                      *tr_user;    /*< [?..?] User-provided closure. */
#define TERMINAL_FLAG_NONE  0x00000000
#define TERMINAL_FLAG_FREE  0x00000001 /*< Set if 'term_delete' should 'free(self)'. */
 uint32_t                   tr_flags;   /*< Terminal flags. */
 __atomic uint32_t          tr_lock;    /*< Spinlock (Used internally). */
 struct term_operations     tr_ops;     /*< Terminal operations. */
 struct term_rgba           tr_fg;      /*< Current foreground color. */
 struct term_rgba           tr_bg;      /*< Current background color. */
#define TERM_PALETTE(fg,bg) (((bg) << 4)|(fg))
 struct term_palette const *tr_palette_sel; /*< [1..1] Selected palette. */
 uint8_t                    tr_palette_idx; /*< Selected palette index (0xf0: bg; 0x0f: fg). */
#define TERM_MOUSEON_NO         0
#define TERM_MOUSEON_YES        1
#define TERM_MOUSEON_WITHMOTION 2 /*< Report motion of pressed buttons. */
 uint8_t                    tr_mouseon;
 uint16_t                   tr_attrib; /*< Text mode attributes (aka. Display flags). */
 coord_t                    tr_savex;
 coord_t                    tr_savey;
 uint8_t                    tr_escape; /*< Current escape mode. */
 uint8_t                    tr_padding2[3];
 char                      *tr_buffer; /*< [1..1][owned] ANSI escape buffer. */
 char                      *tr_bufpos; /*< [1..1] Current R/W position within the buffer (Undefined when not escaping). */
 char                      *tr_bufend; /*< [1..1] End of the ANSI escape buffer (NOTE: Hint this points to a ZERO-character). */
 size_t                     tr_argc;   /*< Actual amount of arguments (Undefined when not escaping). */
 size_t                     tr_arga;   /*< [!0] Allocated amount of arguments. */
 char                     **tr_argv;   /*< [1..1][0..tr_argc][alloc(tr_arga)] Vector of ZERO-terminated ANSI escape arguments (point into 'tr_buffer'). */
union{
 struct term_rgba          *tr_image;  /*< [0..1][owned] Buffer for cell images. */
 uint8_t                   *tr_imabg;  /*< [1..1] Image start pointer (Undefined when 'tr_escape' != 6). */
};
 uint8_t                   *tr_imgps;  /*< [1..1] Image Position (Undefined when 'tr_escape' != 6). */
 uint8_t                   *tr_imgnd;  /*< [1..1] Image end (Undefined when 'tr_escape' != 6). */
};

extern __wunused struct term *
term_new(struct term *buf, struct term_palette const *palette,
         struct term_operations const *ops, void *closure);
extern int term_delete(struct term *self);

extern __nonnull((1)) void term_outc(struct term *self, char ch);
extern __nonnull((1)) void term_outs(struct term *self, char const *s);
extern __nonnull((1)) void term_outn(struct term *self, char const *p, size_t max_chars);
extern __nonnull((1)) void term_outl(struct term *self, void const *p, size_t len);
extern __nonnull((1)) void term_outf(struct term *self, char const *fmt, ...);
extern __nonnull((1)) void term_voutf(struct term *self, char const *fmt, va_list args);

#define ANSI_ESCAPE    '\033' /* '\e' */
#define ANSI_ESCAPE_S  "\033" /* "\e" */


// Special terminal characters
#ifndef __KERNEL__
#define TERM_BELL     '\07'
#define TERM_BELL_S   "\07"
#endif
#define TERM_CR       '\r'
#define TERM_CR_S     "\r"
#define TERM_LF       '\n'
#define TERM_LF_S     "\n"
#define TERM_BACK     '\b'
#define TERM_BACK_S   "\b"
#define TERM_ESCAPE   ANSI_ESCAPE  
#define TERM_ESCAPE_S ANSI_ESCAPE_S
#define TERM_TAB      '\t'
#define TERM_TAB_S    "\t"

#define TERM_TABSIZE  8 /*< Default tab size used by terminal drivers. */



/* Escape verify */
#define ANSI_BRACKET         '['
#define ANSI_BRACKET_RIGHT   ']'
#define ANSI_OPEN_PAREN      '('
#define ANSI_BRACKET_S       "["
#define ANSI_BRACKET_RIGHT_S "]"
#define ANSI_OPEN_PAREN_S    "("
/* Anything in this range (should) exit escape mode. */
#define ANSI_LOW    'A'
#define ANSI_HIGH   'z'
/* Escape commands */
#define ANSI_CUU       'A' /*< Cursor Up. */
#define ANSI_CUD       'B' /*< Cursor Down. */
#define ANSI_CUF       'C' /*< Cursor Forward. */
#define ANSI_CUB       'D' /*< Cursor Back. */
#define ANSI_CNL       'E' /*< Cursor Next Line. */
#define ANSI_CPL       'F' /*< Cursor Previous Line. */
#define ANSI_CHA       'G' /*< Cursor Horizontal Absolute. */
#define ANSI_CUP       'H' /*< Cursor Position. */
#define ANSI_ED        'J' /*< Erase Data. */
#define ANSI_EL        'K' /*< Erase in Line. */
#define ANSI_SU        'S' /*< Scroll Up. */
#define ANSI_SD        'T' /*< Scroll Down. */
#define ANSI_HVP       'f' /*< Horizontal & Vertical Pos. */
#define ANSI_SGR       'm' /*< Select Graphic Rendition. */
#define ANSI_DSR       'n' /*< Device Status Report. */
#define ANSI_SCP       's' /*< Save Cursor Position. */
#define ANSI_RCP       'u' /*< Restore Cursor Position. */
#define ANSI_HIDE      'l' /*< DECTCEM - Hide Cursor. */
#define ANSI_SHOW      'h' /*< DECTCEM - Show Cursor. */

/* Display flags (text attributes) */
#define ANSI_UNDERLINE 0x0001
#define ANSI_ITALIC    0x0002
#define ANSI_FRAKTUR   0x0004
#define ANSI_DOUBLEU   0x0008
#define ANSI_OVERLINE  0x0010
#define ANSI_BLINK     0x0020
#define ANSI_CROSS     0x0040
#define ANSI_BOX       0x0080

#define TERM_DEFAULT_FG ANSI_CL_WHITE
#define TERM_DEFAULT_BG ANSI_CL_BLACK

// Special characters for box-mode
#define ANSIBOX_VLINE     'x' /* 0x78: | */
#define ANSIBOX_HLINE     'q' /* 0x71: - */
#define ANSIBOX_ULCORNER  'l' /* 0x6C: /- */
#define ANSIBOX_URCORNER  'k' /* 0x6B: -\ */
#define ANSIBOX_LLCORNER  'm' /* 0x6D: \- */
#define ANSIBOX_LRCORNER  'j' /* 0x6A: -/ */

__DECL_END

#endif /* !__LIB_TERM_H__ */
