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
#ifndef __KOS_KERNEL_KEYBOARD_C__
#define __KOS_KERNEL_KEYBOARD_C__ 1

#include <kos/config.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/keyboard.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/io.h>
#include <kos/syslog.h>

__DECL_BEGIN

struct kaddist keyboard_input =
 KADDIST_INIT(sizeof(struct kbevent),16);


#define PS2_DATA   0x60
#define PS2_STATUS 0x64
#define PS2_CMD    0x64

#define PS2_STATUS_OUTFULL              0x01
#define PS2_STATUS_INFULL               0x02
#define PS2_STATUS_SYSTEM               0x04
#define PS2_STATUS_IN_IS_CONTROLLER_CMD 0x08
#define PS2_STATUS_KB_ENABLED           0x10
#define PS2_STATUS_SEND_TIMEOUT_ERROR   0x20
#define PS2_STATUS_RECV_TIMEOUT_ERROR   0x40
#define PS2_STATUS_PARITY_ERROR         0x80

#define PS2_CMD_READCFGBYTE    0x20 /* Into 'PS2_DATA' (Set of 'PS2_CFG_*') */
#define PS2_CMD_WRITECFGBYTE   0x60 /* From 'PS2_DATA' (Set of 'PS2_CFG_*') */
#define PS2_CMD_DISABLE_SECOND 0xA7
#define PS2_CMD_ENABLE_SECOND  0xA8
#define PS2_CMD_TEST_SECOND    0xA9 /* Into 'PS2_DATA'; One of 'PS2_TEST_*' */
#define PS2_CMD_DISABLE_FIRST  0xAD
#define PS2_CMD_ENABLE_FIRST   0xAE
#define PS2_CMD_TEST_FIRST     0xAB /* Into 'PS2_DATA'; One of 'PS2_TEST_*' */

#define PS2_CFG_NOCLOCK_FIRST  (1 << 4)
#define PS2_CFG_NOCLOCK_SECOND (1 << 5)

#define PS2_TEST_OK        0x00
#define PS2_TEST_CLOCKLOW  0x01
#define PS2_TEST_CLOCKHIGH 0x02
#define PS2_TEST_DATELOW   0x03
#define PS2_TEST_DATEHIGH  0x04

#define ACK    0xFA
#define RESEND 0xFE


#define LED_SCROLLLOCK (1 << 0)
#define LED_NUMLOCK    (1 << 1)
#define LED_CAPSLOCK   (1 << 2)

__local int set_led(int led, int state) {
 static __u8 led_state = 0; __u8 newstate;
 newstate = state ? led_state|led : led_state&~(led);
 if (newstate == led_state) return 1;
 while (inb(PS2_STATUS)&PS2_STATUS_INFULL);
 outb_p(PS2_DATA,0xed);
 while (inb(PS2_STATUS)&PS2_STATUS_INFULL);
 outb_p(PS2_DATA,newstate);
 if (inb_p(PS2_DATA) == 0xfa) {
  led_state = newstate;
  return 1;
 }
 return 0;
}

__local kbkey_t kb_scan2key(kbscan_t scan, kbstate_t state) {
 kbkey_t result;
 if (state&(KBSTATE_LCTRL|KBSTATE_RCTRL) &&
     state&(KBSTATE_LALT|KBSTATE_RALT)) {
  result = kkeymap_current->km_alt[scan&0x7f];
 } else if (((state&(KBSTATE_LSHIFT|KBSTATE_RSHIFT)) != 0) ^
            ((state&(KBSTATE_CAPSLOCK)) != 0)) {
  result = kkeymap_current->km_upper[scan&0x7f];
 } else {
  result = kkeymap_current->km_lower[scan&0x7f];
 }
 if (result && scan&0x80) result |= KEY_RELEASE;
 return result;
}


static kbstate_t kb_state = KBSTATE_INSERT; /*< Current keyboard state. */
void keyboard_sendscan(kbscan_t scan) {
 struct kbevent event;
 event.e_scan = scan;
 event.e_key = kb_scan2key(scan,kb_state);
 event.e_state = kb_state;
 switch (event.e_key) {
  case KEY_LCTRL             : kb_state |=  (KBSTATE_LCTRL); break;
  case KEY_RCTRL             : kb_state |=  (KBSTATE_RCTRL); break;
  case KEY_LSHIFT            : kb_state |=  (KBSTATE_LSHIFT); break;
  case KEY_RSHIFT            : kb_state |=  (KBSTATE_RSHIFT); break;
  case KEY_LALT              : kb_state |=  (KBSTATE_LALT); break;
  case KEY_RALT              : kb_state |=  (KBSTATE_RALT); break;
  case KEY_LGUI              : kb_state |=  (KBSTATE_LGUI); break;
  case KEY_RGUI              : kb_state |=  (KBSTATE_RGUI); break;
  case KEY_RELEASE|KEY_LCTRL : kb_state &= ~(KBSTATE_LCTRL); break;
  case KEY_RELEASE|KEY_RCTRL : kb_state &= ~(KBSTATE_RCTRL); break;
  case KEY_RELEASE|KEY_LSHIFT: kb_state &= ~(KBSTATE_LSHIFT); break;
  case KEY_RELEASE|KEY_RSHIFT: kb_state &= ~(KBSTATE_RSHIFT); break;
  case KEY_RELEASE|KEY_LALT  : kb_state &= ~(KBSTATE_LALT); break;
  case KEY_RELEASE|KEY_RALT  : kb_state &= ~(KBSTATE_RALT); break;
  case KEY_RELEASE|KEY_LGUI  : kb_state &= ~(KBSTATE_LGUI); break;
  case KEY_RELEASE|KEY_RGUI  : kb_state &= ~(KBSTATE_RGUI); break;
  case KEY_INSERT            : kb_state ^=  (KBSTATE_INSERT); break;
  case KEY_CAPSLOCK          : kb_state ^=  (KBSTATE_CAPSLOCK);   set_led(LED_CAPSLOCK,  kb_state&KBSTATE_CAPSLOCK);   break;
  case KEY_NUMLOCK           : kb_state ^=  (KBSTATE_NUMLOCK);    set_led(LED_NUMLOCK,   kb_state&KBSTATE_NUMLOCK);    break;
  case KEY_SCROLLLOCK        : kb_state ^=  (KBSTATE_SCROLLLOCK); set_led(LED_SCROLLLOCK,kb_state&KBSTATE_SCROLLLOCK); break;
  case KEY_F(12)             : tb_print(); break;
  default: break;
 }

 /* Broadcast the keyboard event through the data distributer
  * NOTE: This needs to be asynchronous because we can't use
  *       preemption as described in the doc of 'kirq_sethandler' */
 kaddist_vsend(&keyboard_input,&event);
}


static void kb_irq_handler(struct kirq_registers *__restrict regs) {
 kbscan_t scan;
 /* Wait for the keyboard to become ready */
 while (!(inb(PS2_STATUS) & PS2_STATUS_OUTFULL));
 /* Read the scancode */
 scan = inb(PS2_DATA);
 switch (scan) {
  case 0x00: case 0xfa:
  case 0xfe: case 0xff: return;
 }
 keyboard_sendscan(scan);
}


static void kernel_initialize_bklayout(void);

void kernel_initialize_keyboard(void) {
 k_syslogf(KLOG_INFO,"[init] Keyboard...\n");
 kirq_sethandler(33,&kb_irq_handler);
 inb(PS2_DATA);
 kernel_initialize_bklayout();
}


static struct kkeymap keymaps[] = {

#define A  (__uchar_t)
 {"en_US",{
   KEY_NONE,27,A'1',A'2',A'3',A'4',A'5',A'6',A'7',A'8',A'9',A'0',A'-',A'=',A'\b',A'\t',
   'q',A'w',A'e',A'r',A't',A'y',A'u',A'i',A'o',A'p',A'[',A']',A'\n',KEY_LCTRL,
   'a',A's',A'd',A'f',A'g',A'h',A'j',A'k',A'l',A';',A'\'',A'`',KEY_LSHIFT,
   '\\',A'z',A'x',A'c',A'v',A'b',A'n',A'm',A',',A'.',A'/',KEY_RSHIFT,
   '*',KEY_LALT,A' ',KEY_CAPSLOCK,KEY_F(1),KEY_F(2),KEY_F(3),KEY_F(4),KEY_F(5),KEY_F(6),KEY_F(7),
   KEY_F(8),KEY_F(9),KEY_F(10),KEY_NUMLOCK,KEY_SCROLLLOCK,KEY_HOME,KEY_UP,KEY_PAGEUP,A'-',KEY_LEFT,0,KEY_RIGHT,A'+',
   KEY_END,KEY_DOWN,KEY_PAGEDOWN,KEY_INSERT,KEY_DELETE,0,0,0,KEY_F(11),KEY_F(12),0, /* everything else */
 },{
   KEY_NONE,27,A'!',A'@',A'#',A'$',A'%',A'^',A'&',A'*',A'(',A')',A'_',A'+',A'\b',A'\t',
   'Q',A'W',A'E',A'R',A'T',A'Y',A'U',A'I',A'O',A'P',A'{',A'}',A'\n',KEY_LCTRL,
   'A',A'S',A'D',A'F',A'G',A'H',A'J',A'K',A'L',A':',A'"',A'~',KEY_LSHIFT,
   '|',A'Z',A'X',A'C',A'V',A'B',A'N',A'M',A'<',A'>',A'?',KEY_RSHIFT,
   '*',KEY_LALT,A' ',KEY_CAPSLOCK,KEY_F(1),KEY_F(2),KEY_F(3),KEY_F(4),KEY_F(5),KEY_F(6),KEY_F(7),
   KEY_F(8),KEY_F(9),KEY_F(10),KEY_NUMLOCK,KEY_SCROLLLOCK,KEY_HOME,KEY_UP,KEY_PAGEUP,A'-',KEY_LEFT,0,KEY_RIGHT,A'+',
   KEY_END,KEY_DOWN,KEY_PAGEDOWN,KEY_INSERT,KEY_DELETE,0,0,0,KEY_F(11),KEY_F(12),0, /* everything else */
 },{
   KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,
   KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,
   KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,
   KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,
   KEY_NONE,KEY_LCTRL,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,
   KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,
   KEY_LSHIFT,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,
   KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_RSHIFT,KEY_NONE,
   KEY_LALT,KEY_NONE,KEY_CAPSLOCK,KEY_F(1),KEY_F(2),KEY_F(3),KEY_F(4),KEY_F(5),KEY_F(6),KEY_F(7),
   KEY_F(8),KEY_F(9),KEY_F(10),KEY_NUMLOCK,KEY_SCROLLLOCK,KEY_HOME,KEY_UP,KEY_PAGEUP,KEY_NONE,KEY_LEFT,0,KEY_RIGHT,KEY_NONE,
   KEY_END,KEY_DOWN,KEY_PAGEDOWN,KEY_INSERT,KEY_CTRLALTDEL,0,0,0,KEY_F(11),KEY_F(12),0, /* everything else */
 }},

 {"de_DE",{
   KEY_NONE,27,A'1',A'2',A'3',A'4',A'5',A'6',A'7',A'8',A'9',A'0',A'ß',A'´',A'\b',A'\t',
   'q',A'w',A'e',A'r',A't',A'z',A'u',A'i',A'o',A'p',A'ü',A'+',A'\n',KEY_LCTRL,
   'a',A's',A'd',A'f',A'g',A'h',A'j',A'k',A'l',A'ö',A'ä',KEY_NONE,KEY_LSHIFT,
   '#',A'y',A'x',A'c',A'v',A'b',A'n',A'm',A',',A'.',A'-',KEY_RSHIFT,
   '*',KEY_LALT,A' ',KEY_CAPSLOCK,KEY_F(1),KEY_F(2),KEY_F(3),KEY_F(4),KEY_F(5),KEY_F(6),KEY_F(7),
   KEY_F(8),KEY_F(9),KEY_F(10),KEY_NUMLOCK,KEY_SCROLLLOCK,KEY_HOME,KEY_UP,KEY_PAGEUP,A'-',KEY_LEFT,0,KEY_RIGHT,A'+',
   KEY_END,KEY_DOWN,KEY_PAGEDOWN,KEY_INSERT,KEY_DELETE,0,0,'<',KEY_F(11),KEY_F(12),0, /* everything else */
 },{
   KEY_NONE,27,A'!',A'"',A'§',A'$',A'%',A'&',A'/',A'(',A')',A'=',A'?',A'`',A'\b',A'\t',
   'Q',A'W',A'E',A'R',A'T',A'Z',A'U',A'I',A'O',A'P',A'Ü',A'*',A'\n',KEY_LCTRL,
   'A',A'S',A'D',A'F',A'G',A'H',A'J',A'K',A'L',A'Ö',A'Ä',KEY_NONE,KEY_LSHIFT,
   '\'',A'Y',A'X',A'C',A'V',A'B',A'N',A'M',A';',A':',A'_',KEY_RSHIFT,
   '*',KEY_LALT,A' ',KEY_CAPSLOCK,KEY_F(1),KEY_F(2),KEY_F(3),KEY_F(4),KEY_F(5),KEY_F(6),KEY_F(7),
   KEY_F(8),KEY_F(9),KEY_F(10),KEY_NUMLOCK,KEY_SCROLLLOCK,KEY_HOME,KEY_UP,KEY_PAGEUP,A'-',KEY_LEFT,0,KEY_RIGHT,A'+',
   KEY_END,KEY_DOWN,KEY_PAGEDOWN,KEY_INSERT,KEY_DELETE,0,0,'>',KEY_F(11),KEY_F(12),0, /* everything else */
 },{
   KEY_NONE,27,KEY_NONE,A'²',A'³',KEY_NONE,KEY_NONE,KEY_NONE,A'{',A'[',A']',A'}',A'\\',KEY_NONE,KEY_NONE,KEY_NONE,
   '@',KEY_NONE,A'€',KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,A'~',KEY_NONE,KEY_LCTRL,
   KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_LSHIFT,
   KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,KEY_NONE,A'µ',KEY_NONE,KEY_NONE,KEY_NONE,KEY_RSHIFT,
   KEY_NONE,KEY_LALT,KEY_NONE,KEY_CAPSLOCK,KEY_F(1),KEY_F(2),KEY_F(3),KEY_F(4),KEY_F(5),KEY_F(6),KEY_F(7),
   KEY_F(8),KEY_F(9),KEY_F(10),KEY_NUMLOCK,KEY_SCROLLLOCK,KEY_HOME,KEY_UP,KEY_PAGEUP,A'-',KEY_LEFT,0,KEY_RIGHT,A'+',
   KEY_END,KEY_DOWN,KEY_PAGEDOWN,KEY_INSERT,KEY_CTRLALTDEL,0,0,'|',KEY_F(11),KEY_F(12),0, /* everything else */
 }},

};
#undef A


struct kkeymap const *kkeymap_current = &keymaps[0];
void kernel_initialize_bklayout(void) {
 char const *layout = getenv("layout");
 struct kkeymap const *iter,*end;
 if (!layout) layout = "de_DE"; /* Sorry, but I'm german... */
 end = (iter = keymaps)+__compiler_ARRAYSIZE(keymaps);
 for (; iter != end; ++iter) {
  if (!strcmp(layout,iter->km_name)) {
   kkeymap_current = iter;
   return;
  }
 }
 k_syslogf(KLOG_ERROR,"Unknown keyboard layout: %q\n",layout);
}



__DECL_END

#endif /* !__KOS_KERNEL_KEYBOARD_C__ */
