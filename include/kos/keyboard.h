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
#ifndef __KOS_KEYBOARD_H__
#define __KOS_KEYBOARD_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

typedef __u8  kbscan_t;  /*< Scancode. */
typedef __u16 kbkey_t;   /*< Keycode. */
typedef __u16 kbstate_t; /*< Keyboard state. */

#define KBSTATE_ISCTRL(x)  (((x)&(KBSTATE_LCTRL|KBSTATE_RCTRL))!=0)
#define KBSTATE_ISSHIFT(x) (((x)&(KBSTATE_LSHIFT|KBSTATE_RSHIFT))!=0)
#define KBSTATE_ISALT(x)   (((x)&(KBSTATE_LALT|KBSTATE_RALT))!=0)
#define KBSTATE_ISGUI(x)   (((x)&(KBSTATE_LGUI|KBSTATE_RGUI))!=0)
#define KBSTATE_NONE       0x0000
#define KBSTATE_LCTRL      0x0001
#define KBSTATE_RCTRL      0x0002
#define KBSTATE_LSHIFT     0x0004
#define KBSTATE_RSHIFT     0x0008
#define KBSTATE_LALT       0x0010
#define KBSTATE_RALT       0x0020
#define KBSTATE_LGUI       0x0040
#define KBSTATE_RGUI       0x0080
#define KBSTATE_INSERT     0x0100 /*< Text fields should insert characters, instead of overwriting existing ones. */
#define KBSTATE_CAPSLOCK   0x2000
#define KBSTATE_NUMLOCK    0x4000
#define KBSTATE_SCROLLLOCK 0x8000

struct kbevent {
 kbscan_t  e_scan;  /*< Scancode. */
 kbkey_t   e_key;   /*< Keycode. */
 kbstate_t e_state; /*< Keyboard state before the key press. */
};


struct kkeymap {
 char  km_name[16];
 __u16 km_lower[128]; /*< Regular stroke. */
 __u16 km_upper[128]; /*< Shift. */
 __u16 km_alt[128];   /*< Alt-GR/Control+Alt. */
};

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if a given keycode describes the
// pressing (down) of the specific key, or FALSE(0) otherwise.
#define KEY_ISDOWN(x)   (!((x)&KEY_RELEASE))

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if a given keycode describes the
// releasing (up) of the specific key, or FALSE(0) otherwise.
#define KEY_ISUP(x)       ((x)&KEY_RELEASE)

//////////////////////////////////////////////////////////////////////////
// Force a keycode to becode UP, or down (s.a.: 'KEY_ISDOWN' & 'KEY_ISUP')
#define KEY_TODOWN(x)   ((x)&~(KEY_RELEASE))
#define KEY_TOUP(x)       ((x)|KEY_RELEASE)

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if a given keycode describes a ASCII character, or FALSE(0) otherwise.
// A ASCII keycode can be converted into a character through use of 'KEY_TOASCII'
#define KEY_ISASCII(x)  (!((x)&0x7f80))

//////////////////////////////////////////////////////////////////////////
// Returns the ASCII Character for a given keycode that has compared
// TRUE(1) for being a ASCII character (s.a.: 'KEY_ISASCII')
#define KEY_TOASCII(x)    ((x)&0x7f)

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if a given keycode describes a UTF-8 character, or FALSE(0) otherwise.
// A UTF-8 keycode can be converted into a character through use of 'KEY_TOUTF8'
#define KEY_ISUTF8(x)   (!((x)&0x7f00))

//////////////////////////////////////////////////////////////////////////
// Returns the UTF-8 Character for a given keycode that has compared
// TRUE(1) for being a UTF-8 character (s.a.: 'KEY_ISUTF8')
#define KEY_TOUTF8(x)     ((x)&0xff)

//////////////////////////////////////////////////////////////////////////
// Returns the non-pressed key-code of a given ASCII/UTF-8 Character.
#define KEY_CHR(x)        ((x)&0xff)


//////////////////////////////////////////////////////////////////////////
// Keys always represented through control characters.
#define KEY_ESC        '\033'
#define KEY_TAB        '\t'
#define KEY_BACKSPACE  '\b'
#define KEY_RETURN     '\n'

//////////////////////////////////////////////////////////////////////////
// Special (non-character-based) keys
// NOTE: All of these keys describe the DOWN (pressed) codes, with
//       each owning an additional UP-code that can be generated
//       at compile-time through use of 'KEY_TOUP'. e.g.:
//       >> kbkey_t key;
//       >> int fd = open("/dev/kbkey",O_RDONLY);
//       >> while (read(fd,&key,sizeof(key)) == sizeof(key)) switch (key) {
//       >>   case KEY_TOUP(KEY_LCTRL): printf("Left control released\n"); break;
//       >>   case KEY_TOUP(KEY_RCTRL): printf("Right control released\n"); break;
//       >>   default: break;
//       >> }
#define KEY_NONE       0      /*< Used internally, but never returned in events. - Can safely be used as error-value. */
#define KEY_RELEASE    0x8000 /*< FLAG: The key was released. */
#define KEY_LCTRL      0x0101 /*< Left control. */
#define KEY_RCTRL      0x0102 /*< Right control. */
#define KEY_LSHIFT     0x0103 /*< Left shift. */
#define KEY_RSHIFT     0x0104 /*< Right shift. */
#define KEY_LALT       0x0105 /*< Left alt. */
#define KEY_RALT       0x0106 /*< Right alt. */
#define KEY_LGUI       0x0107 /*< Left gui-key (windows/super). */
#define KEY_RGUI       0x0108 /*< Right gui-key (windows/super). */
#define KEY_CAPSLOCK   0x0109 /*< Caps-lock key. */
#define KEY_NUMLOCK    0x010A /*< Num-lock key. */
#define KEY_SCROLLLOCK 0x010B /*< Scroll-lock key. */
#define KEY_HOME       0x010C /*< Home/Pos-1. */
#define KEY_END        0x010D /*< End. */
#define KEY_PAGEUP     0x010E /*< Page up. */
#define KEY_PAGEDOWN   0x010F /*< Page down. */
#define KEY_INSERT     0x0110 /*< Insert. */
#define KEY_DELETE     0x0111 /*< Delete/Erase/Del/Entf (remove character after cursor). */
#define KEY_UP         0x0150 /*< Arrow-key up. */
#define KEY_LEFT       0x0151 /*< Arrow-key left. */
#define KEY_DOWN       0x0152 /*< Arrow-key down. */
#define KEY_RIGHT      0x0153 /*< Arrow-key right. */
#define KEY_CTRLALTDEL 0x0154 /*< Ctrl+Alt+Del */
#define KEY_F(n)      (0x7000+((n)-1))

#define KEY_F1   KEY_F(1)
#define KEY_F2   KEY_F(2)
#define KEY_F3   KEY_F(3)
#define KEY_F4   KEY_F(4)
#define KEY_F5   KEY_F(5)
#define KEY_F6   KEY_F(6)
#define KEY_F7   KEY_F(7)
#define KEY_F8   KEY_F(8)
#define KEY_F9   KEY_F(9)
#define KEY_F10  KEY_F(10)
#define KEY_F11  KEY_F(11)
#define KEY_F12  KEY_F(12)


/* Key name aliases */
#define KEY_ARROW_UP    KEY_UP
#define KEY_ARROW_DOWN  KEY_DOWN
#define KEY_ARROW_RIGHT KEY_RIGHT
#define KEY_ARROW_LEFT  KEY_LEFT
#define KEY_PAGE_UP     KEY_PAGEUP
#define KEY_PAGE_DOWN   KEY_PAGEDOWN
#define KEY_DEL         KEY_DELETE
#define KEY_ENTER       KEY_RETURN


__DECL_END

#endif /* !__KOS_KEYBOARD_H__ */
