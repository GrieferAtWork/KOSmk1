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
#ifndef __HW_VIDEO_VGA_H__
#define __HW_VIDEO_VGA_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/errno.h>
#include <sys/io.h>

__DECL_BEGIN

/* NOTE: Changes have been made to the original content.
 *       These changes are plainly marked as such. */

/* Some of the code below is taken from SVGAlib.  The original,
 * unmodified copyright notice for that code is below. */
/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/* VGA data register ports */
#define VGA_CRT_DC   0x3D5 /*< CRT Controller Data Register - color emulation. */
#define VGA_CRT_DM   0x3B5 /*< CRT Controller Data Register - mono emulation. */
#define VGA_ATT_R    0x3C1 /*< Attribute Controller Data Read Register. */
#define VGA_ATT_W    0x3C0 /*< Attribute Controller Data Write Register. */
#define VGA_GFX_D    0x3CF /*< Graphics Controller Data Register. */
#define VGA_SEQ_D    0x3C5 /*< Sequencer Data Register. */
#define VGA_MIS_R    0x3CC /*< Misc Output Read Register. */
#define VGA_MIS_W    0x3C2 /*< Misc Output Write Register. */
#define VGA_FTC_R    0x3CA /*< Feature Control Read Register. */
#define VGA_IS1_RC   0x3DA /*< Input Status Register 1 - color emulation (Set of 'VGA_IS1_*'). */
#define VGA_IS1_RM   0x3BA /*< Input Status Register 1 - mono emulation (Set of 'VGA_IS1_*'). */
#define VGA_PEL_D    0x3C9 /*< PEL Data Register. */
#define VGA_PEL_MSK  0x3C6 /*< PEL mask register. */

/* EGA-specific registers */
#define EGA_GFX_E0   0x3CC /*< Graphics enable processor 0. */
#define EGA_GFX_E1   0x3CA /*< Graphics enable processor 1. */

/* VGA index register ports */
#define VGA_CRT_IC   0x3D4 /*< CRT Controller Index - color emulation. */
#define VGA_CRT_IM   0x3B4 /*< CRT Controller Index - mono emulation. */
#define VGA_ATT_IW   0x3C0 /*< Attribute Controller Index & Data Write Register. */
#define VGA_GFX_I    0x3CE /*< Graphics Controller Index. */
#define VGA_SEQ_I    0x3C4 /*< Sequencer Index. */
#define VGA_PEL_IW   0x3C8 /*< PEL Write Index. */
#define VGA_PEL_IR   0x3C7 /*< PEL Read Index. */

/* standard VGA indexes max counts */
#define VGA_CRT_C    0x19  /*< Number of CRT Controller Registers. */
#define VGA_ATT_C    0x15  /*< Number of Attribute Controller Registers. */
#define VGA_GFX_C    0x09  /*< Number of Graphics Controller Registers. */
#define VGA_SEQ_C    0x05  /*< Number of Sequencer Registers. */
#define VGA_MIS_C    0x01  /*< Number of Misc Output Register. */

/* VGA status register bit masks (Added by GrieferAtWork). */
#define VGA_IS1_DISP_ENABLED 0x01 /*< The scanpoint is currently on-screen (On a CRT that is the light pointer). */
#define VGA_IS1_V_RETRACE    0x08 /*< Set while the VGA adapter is syncing memory to screen (Added by GrieferAtWork). */

/* VGA misc register bit masks */
#define VGA_MIS_COLOR          0x01
#define VGA_MIS_ENB_MEM_ACCESS 0x02
#define VGA_MIS_DCLK_28322_720 0x04
#define VGA_MIS_ENB_PLL_LOAD  (0x04 | 0x08)
#define VGA_MIS_SEL_HIGH_PAGE  0x20
#define VGA_MIS_CLOCK_25MHZ    0x00 /*< Added by GrieferAtWork. */
#define VGA_MIS_CLOCK_28MHZ    0x04 /*< Added by GrieferAtWork. */
#define VGA_MIS_CLOCK_SHIFT       2 /*< Added by GrieferAtWork. */
#define VGA_MIS_CLOCK_MASK     0x0C /*< Added by GrieferAtWork. */

/* VGA CRT controller register indices */
#define VGA_CRTC_H_TOTAL       0x00 /*< Horizontal total. */
#define VGA_CRTC_H_DISP        0x01 /*< Horizontal display enable end. */
#define VGA_CRTC_H_BLANK_START 0x02 /*< Horizontal blank start. */
#define VGA_CRTC_H_BLANK_END   0x03 /*< Horizontal blank end. */
#define VGA_CRTC_H_SYNC_START  0x04 /*< Horizontal sync/retrace start. */
#define VGA_CRTC_H_SYNC_END    0x05 /*< Horizontal sync/retrace end. */
#define VGA_CRTC_V_TOTAL       0x06 /*< Vertical total. */
#define VGA_CRTC_OVERFLOW      0x07 /*< Overflow register. */
#define VGA_CRTC_PRESET_ROW    0x08 /*< Preset row scan. */
#define VGA_CRTC_MAX_SCAN      0x09 /*< Max scan line. */
#define VGA_CRTC_CURSOR_START  0x0A
#define VGA_CRTC_CURSOR_END    0x0B
#define VGA_CRTC_START_HI      0x0C
#define VGA_CRTC_START_LO      0x0D
#define VGA_CRTC_CURSOR_HI     0x0E
#define VGA_CRTC_CURSOR_LO     0x0F
#define VGA_CRTC_V_SYNC_START  0x10 /*< Vertical sync/retrace start. */
#define VGA_CRTC_V_SYNC_END    0x11 /*< Vertical sync/retrace end. */
#define VGA_CRTC_V_DISP_END    0x12 /*< Vertical display enable end. */
#define VGA_CRTC_OFFSET        0x13 /*< Logical width (Aka offset). */
#define VGA_CRTC_UNDERLINE     0x14 /*< Underline location. */
#define VGA_CRTC_V_BLANK_START 0x15 /*< Vertical blank start. */
#define VGA_CRTC_V_BLANK_END   0x16 /*< Vertical blank end. */
#define VGA_CRTC_MODE          0x17 /*< Mode control. */
#define VGA_CRTC_LINE_COMPARE  0x18
#define VGA_CRTC_REGS          VGA_CRT_C

/* VGA CRT controller bit masks */
#define VGA_CR03_H_BLANK_END_SHIFT       0 /*< Horizontal blanking (bits 0..4) (spacing on the right) (Added by GrieferAtWork). */
#define VGA_CR03_H_BLANK_END            (0x1f << VGA_CR03_H_BLANK_END_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR03_H_SKEW_SHIFT            5 /*< Horizontal display SKEW (spacing on the left) (Added by GrieferAtWork). */
#define VGA_CR03_H_SKEW                 (0x3 << VGA_CR03_H_SKEW_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR03_PRESERVE                0x80 /*< Bits that should be preserved when setting/ignored when reading (Added by GrieferAtWork). */
#define VGA_CR05_H_SYNC_END_SHIFT        0 /*< Different between this and 'VGA_CR03_H_BLANK_END' is space on the right (Added by GrieferAtWork). */
#define VGA_CR05_H_SYNC_END             (0x1f << VGA_CR05_H_SYNC_END_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR05_H_BLANK_END_B5_SHIFT    7 /*< Bit #5 for 'VGA_CR03_H_BLANK_END' (Added by GrieferAtWork). */
#define VGA_CR05_H_BLANK_END_B5         (1 << VGA_CR05_H_BLANK_END_B5_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR05_PRESERVE                0x60 /*< Bits that should be preserved when setting/ignored when reading (Added by GrieferAtWork). */
#define VGA_CR07_V_TOTAL_B8_SHIFT        0 /*< Bit #8 for 'VGA_CRTC_V_TOTAL' (Added by GrieferAtWork). */
#define VGA_CR07_V_DISP_END_B8_SHIFT     1 /*< Bit #8 for 'VGA_CRTC_V_DISP_END' (Added by GrieferAtWork). */
#define VGA_CR07_V_SYNC_START_B8_SHIFT   2 /*< Bit #8 for 'VGA_CRTC_V_SYNC_START' (Added by GrieferAtWork). */
#define VGA_CR07_V_BLANK_START_B8_SHIFT  3 /*< Bit #8 for 'VGA_CRTC_V_BLANK_START' (Added by GrieferAtWork). */
#define VGA_CR07_V_TOTAL_B9_SHIFT        5 /*< Bit #9 for 'VGA_CRTC_V_TOTAL' (Added by GrieferAtWork). */
#define VGA_CR07_V_DISP_END_B9_SHIFT     6 /*< Bit #9 for 'VGA_CRTC_V_DISP_END' (Added by GrieferAtWork). */
#define VGA_CR07_V_SYNC_START_B9_SHIFT   7 /*< Bit #9 for 'VGA_CRTC_V_SYNC_START' (Added by GrieferAtWork). */
#define VGA_CR07_V_TOTAL_B8             (1 << VGA_CR07_V_TOTAL_B8_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR07_V_DISP_END_B8          (1 << VGA_CR07_V_DISP_END_B8_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR07_V_SYNC_START_B8        (1 << VGA_CR07_V_SYNC_START_B8_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR07_V_BLANK_START_B8       (1 << VGA_CR07_V_BLANK_START_B8_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR07_V_TOTAL_B9             (1 << VGA_CR07_V_TOTAL_B9_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR07_V_DISP_END_B9          (1 << VGA_CR07_V_DISP_END_B9_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR07_V_SYNC_START_B9        (1 << VGA_CR07_V_SYNC_START_B9_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR07_PRESERVE                0x10 /*< Bits that should be preserved when setting/ignored when reading (Added by GrieferAtWork). */
#define VGA_CR09_V_BLANK_START_B9_SHIFT  5 /*< Bit #9 for 'VGA_CRTC_V_BLANK_START' (Added by GrieferAtWork). */
#define VGA_CR09_V_BLANK_START_B9       (1 << VGA_CR09_V_BLANK_START_B9_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR09_DOUBLE_WIDE             0x80 /*< Double wide vertical scanlines (Added by GrieferAtWork). */
#define VGA_CR09_PRESERVE                0xdf /*< Added by GrieferAtWork. */
#define VGA_CR11_V_SYNC_END_SHIFT        0 /*< Empty sync space below the screen (Added by GrieferAtWork). */
#define VGA_CR11_V_SYNC_END             (0x7 << VGA_CR11_V_SYNC_END_SHIFT) /*< Added by GrieferAtWork. */
#define VGA_CR11_LOCK_CR0_CR7            0x80 /* lock writes to CR0 - CR7 */
#define VGA_CR17_H_V_SIGNALS_ENABLED     0x80

/* VGA attribute controller register indices */
#define VGA_ATC_PALETTE0     0x00
#define VGA_ATC_PALETTE1     0x01
#define VGA_ATC_PALETTE2     0x02
#define VGA_ATC_PALETTE3     0x03
#define VGA_ATC_PALETTE4     0x04
#define VGA_ATC_PALETTE5     0x05
#define VGA_ATC_PALETTE6     0x06
#define VGA_ATC_PALETTE7     0x07
#define VGA_ATC_PALETTE8     0x08
#define VGA_ATC_PALETTE9     0x09
#define VGA_ATC_PALETTEA     0x0A
#define VGA_ATC_PALETTEB     0x0B
#define VGA_ATC_PALETTEC     0x0C
#define VGA_ATC_PALETTED     0x0D
#define VGA_ATC_PALETTEE     0x0E
#define VGA_ATC_PALETTEF     0x0F
#define VGA_ATC_MODE         0x10
#define VGA_ATC_OVERSCAN     0x11
#define VGA_ATC_PLANE_ENABLE 0x12
#define VGA_ATC_PEL          0x13
#define VGA_ATC_COLOR_PAGE   0x14

#define VGA_AR_ENABLE_DISPLAY 0x20

#define VGA_AT10_8BITPAL     0x80 /*< 8-bit palette index (Added by GrieferAtWork). */
#define VGA_AT10_DUP9        0x04 /*< Duplicate the 8'th text dot into the 9'th when 'VGA_SR01_CHAR_CLK_8DOTS' isn't set, instead of filling it with background (Added by GrieferAtWork). */

/* VGA sequencer register indices */
#define VGA_SEQ_RESET         0x00
#define VGA_SEQ_CLOCK_MODE    0x01
#define VGA_SEQ_PLANE_WRITE   0x02
#define VGA_SEQ_CHARACTER_MAP 0x03
#define VGA_SEQ_MEMORY_MODE   0x04

/* VGA sequencer register bit masks */
#define VGA_SR01_CHAR_CLK_8DOTS 0x01 /* bit 0: character clocks 8 dots wide are generated */
#define VGA_SR01_DOUBLEWIDE	    0x04 /* bit 2: Double wide characters (Added by GrieferAtWork). */
#define VGA_SR01_SCREEN_OFF     0x20 /* bit 5: Screen is off */
#define VGA_SR02_ALL_PLANES     0x0F /* bits 3-0: enable access to all planes */
#define VGA_SR04_EXT_MEM        0x02 /* bit 1: allows complete mem access to 256K */
#define VGA_SR04_SEQ_MODE       0x04 /* bit 2: directs system to use a sequential addressing mode */
#define VGA_SR04_CHN_4M         0x08 /* bit 3: selects modulo 4 addressing for CPU access to display memory */

/* VGA graphics controller register indices */
#define VGA_GFX_SR_VALUE      0x00
#define VGA_GFX_SR_ENABLE     0x01
#define VGA_GFX_COMPARE_VALUE 0x02
#define VGA_GFX_DATA_ROTATE   0x03
#define VGA_GFX_PLANE_READ    0x04
#define VGA_GFX_MODE          0x05 /*< Mode register. */
#define VGA_GFX_MISC          0x06
#define VGA_GFX_COMPARE_MASK  0x07
#define VGA_GFX_BIT_MASK      0x08

/* VGA graphics controller bit masks */
#define VGA_GR06_GRAPHICS_MODE 0x01

/* macro for composing an 8-bit VGA register index and value
 * into a single 16-bit quantity */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#   define __VGA_BUILDWORD(index,value) ((__u16)(index)|((__u16)(value) << 8))
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#   define __VGA_BUILDWORD(index,value) ((__u16)(value)|((__u16)(index) << 8))
#endif

#ifdef __VGA_BUILDWORD
#define VGA_OUT16VAL(v,r) __VGA_BUILDWORD(r,v)
#endif

/* decide whether we should enable the faster 16-bit VGA register writes */
#ifdef VGA_OUT16VAL
#define VGA_OUTW_WRITE
#endif


#ifdef __VGA_BUILDWORD
#   define __VGA_OUTWORD(port,index,value)  outw_p(port,__VGA_BUILDWORD(index,value))
#else
#   define __VGA_OUTWORD(port,index,value) (outb_p(port,index),outb_p((port)+1,value))
#endif

#define vga_io_r      inb_p
#define vga_io_w      outb_p
#define vga_io_w_fast __VGA_OUTWORD



/*
 * VGA CRTC register read/write
 */
__forcelocal __u8 vga_rcrt(__u8 reg) { vga_io_w(VGA_CRT_IC,reg); return vga_io_r(VGA_CRT_DC); }
#if VGA_CRT_IC == VGA_CRT_DC-1
__forcelocal void vga_wcrt(__u8 reg, __u8 val) {
 vga_io_w_fast(VGA_CRT_IC,reg,val);
#else
 vga_io_w(VGA_CRT_IC,reg);
 vga_io_w(VGA_CRT_DC,val);
#endif
}

/*
 * VGA sequencer register read/write
 */
__forcelocal __u8 vga_rseq(__u8 reg) { vga_io_w(VGA_SEQ_I,reg); return vga_io_r(VGA_SEQ_D); }
__forcelocal void vga_wseq(__u8 reg, __u8 val) {
#if VGA_SEQ_I == VGA_SEQ_D-1
 vga_io_w_fast(VGA_SEQ_I,reg,val);
#else
 vga_io_w(VGA_SEQ_I,reg);
 vga_io_w(VGA_SEQ_D,val);
#endif
}


/*
 * VGA graphics controller register read/write
 */
__forcelocal __u8 vga_rgfx(__u8 reg) { vga_io_w(VGA_GFX_I,reg); return vga_io_r(VGA_GFX_D); }
__forcelocal void vga_wgfx(__u8 reg, __u8 val) {
#if VGA_GFX_I == VGA_GFX_D-1
 vga_io_w_fast(VGA_GFX_I,reg,val);
#else
 vga_io_w(VGA_GFX_I,reg);
 vga_io_w(VGA_GFX_D,val);
#endif
}

/*
 * VGA attribute controller register read/write
 */
__forcelocal __u8 vga_rattr(__u8 reg) {
 vga_io_r(VGA_IS1_RC); /* Set to expect-index. */
 vga_io_w(VGA_ATT_W,reg);
 return vga_io_r(VGA_ATT_R);
}
__forcelocal void vga_wattr(__u8 reg, __u8 val) {
 vga_io_r(VGA_IS1_RC); /* Set to expect-index. */
 vga_io_w(VGA_ATT_IW,reg);
 vga_io_w(VGA_ATT_W,val);
}



struct __packed vga_crt_registers {
 __u8 vc_h_total;       /*< VGA_CRTC_H_TOTAL       0x5F 	0x5F 	0x5F 	0x5F. */
 __u8 vc_h_disp;        /*< VGA_CRTC_H_DISP        0x4F 	0x4F 	0x4F 	0x4F. */
 __u8 vc_h_blank_start; /*< VGA_CRTC_H_BLANK_START 0x50 	0x50 	0x50 	0x50. */
 __u8 vc_h_blank_end;   /*< VGA_CRTC_H_BLANK_END   0x82 	0x82 	0x82 	0x82. */
 __u8 vc_h_sync_start;  /*< VGA_CRTC_H_SYNC_START  0x55 	0x54 	0x54 	0x54. */
 __u8 vc_h_sync_end;    /*< VGA_CRTC_H_SYNC_END    0x81 	0x80 	0x80 	0x80. */
 __u8 vc_v_total;       /*< VGA_CRTC_V_TOTAL       0xBF 	0x0B 	0xBF 	0x0D. */
 __u8 vc_overflow;      /*< VGA_CRTC_OVERFLOW      0x1F 	0x3E 	0x1F 	0x3E. */
 __u8 vc_preset_row;    /*< VGA_CRTC_PRESET_ROW    0x00 	0x00 	0x00 	0x00. */
 __u8 vc_max_scan;      /*< VGA_CRTC_MAX_SCAN      0x4F 	0x40 	0x41 	0x41. */
 __u8 vc_cursor_start;  /*< VGA_CRTC_CURSOR_START  ????  ????  ????  ????. */
 __u8 vc_cursor_end;    /*< VGA_CRTC_CURSOR_END    ????  ????  ????  ????. */
 __u8 vc_start_hi;      /*< VGA_CRTC_START_HI      ????  ????  ????  ????. */
 __u8 vc_start_lo;      /*< VGA_CRTC_START_LO      ????  ????  ????  ????. */
 __u8 vc_cursor_hi;     /*< VGA_CRTC_CURSOR_HI     ????  ????  ????  ????. */
 __u8 vc_cursor_lo;     /*< VGA_CRTC_CURSOR_LO     ????  ????  ????  ????. */
 __u8 vc_v_sync_start;  /*< VGA_CRTC_V_SYNC_START  0x9C 	0xEA 	0x9C 	0xEA. */
 __u8 vc_v_sync_end;    /*< VGA_CRTC_V_SYNC_END    0x8E 	0x8C 	0x8E 	0xAC. */
 __u8 vc_v_disp_end;    /*< VGA_CRTC_V_DISP_END    0x8F 	0xDF 	0x8F 	0xDF. */
 __u8 vc_offset;        /*< VGA_CRTC_OFFSET        0x28 	0x28 	0x28 	0x28. */
 __u8 vc_underline;     /*< VGA_CRTC_UNDERLINE     0x1F 	0x00 	0x40 	0x00. */
 __u8 vc_v_blank_start; /*< VGA_CRTC_V_BLANK_START 0x96 	0xE7 	0x96 	0xE7. */
 __u8 vc_v_blank_end;   /*< VGA_CRTC_V_BLANK_END   0xB9 	0x04 	0xB9 	0x06. */
 __u8 vc_mode;          /*< VGA_CRTC_MODE          0xA3 	0xE3 	0xA3 	0xE3. */
 __u8 vc_line_compare;  /*< VGA_CRTC_LINE_COMPARE  ????  ????  ????  ????. */
};

struct __packed vga_att_registers {
 __u8 va_palette[16];   /*< VGA_ATC_PALETTE0..VGA_ATC_PALETTEF. */
 __u8 va_mode;          /*< VGA_ATC_MODE         0x0C 	0x01 	0x41 	0x41. */
 __u8 va_overscan;      /*< VGA_ATC_OVERSCAN     0x00 	0x00 	0x00 	0x00. */
 __u8 va_plane_enable;  /*< VGA_ATC_PLANE_ENABLE 0x0F 	0x0F 	0x0F 	0x0F. */
 __u8 va_pel;           /*< VGA_ATC_PEL          0x08 	0x00 	0x00 	0x00. */
 __u8 va_color_page;    /*< VGA_ATC_COLOR_PAGE   0x00 	0x00 	0x00 	0x00. */
};

struct __packed vga_gfx_registers {
 __u8 vg_sr_value;      /*< VGA_GFX_SR_VALUE      ????  ????  ????  ????. */
 __u8 vg_sr_enable;     /*< VGA_GFX_SR_ENABLE     ????  ????  ????  ????. */
 __u8 vg_compare_value; /*< VGA_GFX_COMPARE_VALUE ????  ????  ????  ????. */
 __u8 vg_data_rotate;   /*< VGA_GFX_DATA_ROTATE   ????  ????  ????  ????. */
 __u8 vg_plane_read;    /*< VGA_GFX_PLANE_READ    ????  ????  ????  ????. */
 __u8 vg_mode;          /*< VGA_GFX_MODE          0x10 	0x00 	0x40 	0x40. */
 __u8 vg_misc;          /*< VGA_GFX_MISC          0x0E 	0x05 	0x05 	0x05. */
 __u8 vg_compare_mask;  /*< VGA_GFX_COMPARE_MASK  ????  ????  ????  ????. */
 __u8 vg_bit_mask;      /*< VGA_GFX_BIT_MASK      ????  ????  ????  ????. */
};

struct __packed vga_seq_registers {
 __u8 vs_reset;         /*< VGA_SEQ_RESET         ????  ????  ????  ????. */
 __u8 vs_clock_mode;    /*< VGA_SEQ_CLOCK_MODE    0x00 	0x01 	0x01 	0x01. */
 __u8 vs_place_write;   /*< VGA_SEQ_PLANE_WRITE   ????  ????  ????  ????. */
 __u8 vs_character_map; /*< VGA_SEQ_CHARACTER_MAP 0x00 	0x00 	0x00 	0x00. */
 __u8 vs_memory_mode;   /*< VGA_SEQ_MEMORY_MODE   0x07 	0x02 	0x0E 	0x06. */
};

struct __packed vga_mis_registers {
 __u8 vm_misc;          /*< VGA_MIS_(R|W)         0x67 	0xE3 	0x63 	0xE3. */
};

struct __packed vgastate {
 union __packed { __u8 vs_crt[VGA_CRT_C]; struct vga_crt_registers vs_crt_regs; };
 union __packed { __u8 vs_att[VGA_ATT_C]; struct vga_att_registers vs_att_regs; };
 union __packed { __u8 vs_gfx[VGA_GFX_C]; struct vga_gfx_registers vs_gfx_regs; };
 union __packed { __u8 vs_seq[VGA_SEQ_C]; struct vga_seq_registers vs_seq_regs; };
 union __packed { __u8 vs_mis[VGA_MIS_C]; struct vga_mis_registers vs_mis_regs; };
};

#ifdef __KERNEL__
extern void kernel_initialize_vga(void);

extern void vgastate_save(struct vgastate *__restrict self);
extern void vgastate_restore(struct vgastate const *__restrict self);
#endif

#define VGAFORMAT_MIS_CLOCK_BITS         2
#define VGAFORMAT_CRT_H_TOTAL_BITS       8
#define VGAFORMAT_CRT_H_DISP_END_BITS    8
#define VGAFORMAT_CRT_H_BLANK_START_BITS 8
#define VGAFORMAT_CRT_H_BLANK_END_BITS   6
#define VGAFORMAT_CRT_H_SYNC_START_BITS  8
#define VGAFORMAT_CRT_H_SYNC_END_BITS    5
#define VGAFORMAT_CRT_H_SKEW_END_BITS    2
#define VGAFORMAT_CRT_V_TOTAL_BITS       10
#define VGAFORMAT_CRT_V_SYNC_START_BITS  10
#define VGAFORMAT_CRT_V_SYNC_END_BITS    4
#define VGAFORMAT_CRT_V_DISP_END_BITS    10
#define VGAFORMAT_CRT_V_BLANK_START_BITS 10
#define VGAFORMAT_CRT_V_BLANK_END_BITS   8

typedef unsigned int vga_clock_t;
typedef unsigned int vga_bytes_t;  /*< Bytes. */
typedef unsigned int vga_pixels_t; /*< Pixels. */
typedef unsigned int vga_hz_t;     /*< ~Someting~/Second. */

#define __SAFE_DOWNCAST(T,result,val) (((val) > (T)-1) ? KE_OVERFLOW : ((result) = (val),KE_OK))
#define __BITS_OVERFLOW(bits,value)   __unlikely((value) >= (1 << (bits)))

__local kerrno_t vgastate_set_crt_h_total(struct vgastate *__restrict self, unsigned int value)       { return __SAFE_DOWNCAST(__u8,self->vs_crt_regs.vc_h_total,value); }
__local kerrno_t vgastate_set_crt_h_disp(struct vgastate *__restrict self, unsigned int value)        { return __SAFE_DOWNCAST(__u8,self->vs_crt_regs.vc_h_disp,value); }
__local kerrno_t vgastate_set_crt_h_blank_start(struct vgastate *__restrict self, unsigned int value) { return __SAFE_DOWNCAST(__u8,self->vs_crt_regs.vc_h_blank_start,value); }
__local kerrno_t vgastate_set_crt_h_sync_start(struct vgastate *__restrict self, unsigned int value)  { return __SAFE_DOWNCAST(__u8,self->vs_crt_regs.vc_h_sync_start,value); }
__local kerrno_t vgastate_set_crt_h_blank_end(struct vgastate *__restrict self, unsigned int value);
__local kerrno_t vgastate_set_crt_h_sync_end(struct vgastate *__restrict self, unsigned int value);
__local kerrno_t vgastate_set_crt_h_skew(struct vgastate *__restrict self, unsigned int value);
__local kerrno_t vgastate_set_crt_v_total(struct vgastate *__restrict self, unsigned int value);
__local kerrno_t vgastate_set_crt_v_sync_start(struct vgastate *__restrict self, unsigned int value);
__local kerrno_t vgastate_set_crt_v_sync_end(struct vgastate *__restrict self, unsigned int value);
__local kerrno_t vgastate_set_crt_v_disp_end(struct vgastate *__restrict self, unsigned int value);
__local kerrno_t vgastate_set_crt_v_blank_start(struct vgastate *__restrict self, unsigned int value);
__local kerrno_t vgastate_set_crt_v_blank_end(struct vgastate *__restrict self, unsigned int value) { return __SAFE_DOWNCAST(__u8,self->vs_crt_regs.vc_v_blank_end,value); }

__local __constcall unsigned int vgastate_get_crt_h_total(struct vgastate const *__restrict self)       { return (unsigned int)self->vs_crt_regs.vc_h_total; }
__local __constcall unsigned int vgastate_get_crt_h_disp(struct vgastate const *__restrict self)        { return (unsigned int)self->vs_crt_regs.vc_h_disp; }
__local __constcall unsigned int vgastate_get_crt_h_blank_start(struct vgastate const *__restrict self) { return (unsigned int)self->vs_crt_regs.vc_h_blank_start; }
__local __constcall unsigned int vgastate_get_crt_h_sync_start(struct vgastate const *__restrict self)  { return (unsigned int)self->vs_crt_regs.vc_h_sync_start; }
__local __constcall unsigned int vgastate_get_crt_h_blank_end(struct vgastate const *__restrict self);
__local __constcall unsigned int vgastate_get_crt_h_sync_end(struct vgastate const *__restrict self);
__local __constcall unsigned int vgastate_get_crt_h_skew(struct vgastate const *__restrict self);
__local __constcall unsigned int vgastate_get_crt_v_total(struct vgastate const *__restrict self);
__local __constcall unsigned int vgastate_get_crt_v_sync_start(struct vgastate const *__restrict self);
__local __constcall unsigned int vgastate_get_crt_v_sync_end(struct vgastate const *__restrict self);
__local __constcall unsigned int vgastate_get_crt_v_disp_end(struct vgastate const *__restrict self);
__local __constcall unsigned int vgastate_get_crt_v_blank_start(struct vgastate const *__restrict self);
__local __constcall unsigned int vgastate_get_crt_v_blank_end(struct vgastate const *__restrict self)  { return (unsigned int)self->vs_crt_regs.vc_v_blank_end; }

__local __constcall vga_clock_t vgastate_get_mis_clock(struct vgastate const *__restrict self);
__local kerrno_t vgastate_set_mis_clock(struct vgastate *__restrict self, vga_clock_t value);

__local __constcall vga_pixels_t vgastate_get_dots_per_char(struct vgastate const *__restrict self) { return (self->vs_seq_regs.vs_clock_mode&VGA_SR01_CHAR_CLK_8DOTS) ? 8 : 9; }
__local __constcall vga_hz_t vgastate_get_clock_hz(struct vgastate const *__restrict self);


__local vga_bytes_t  vgastate_get_pitch(struct vgastate const *__restrict self);
__local vga_pixels_t vgastate_get_dim_x(struct vgastate const *__restrict self);
__local vga_pixels_t vgastate_get_dim_y(struct vgastate const *__restrict self);


#ifndef __INTELLISENSE__
__local kerrno_t
vgastate_set_crt_h_blank_end(struct vgastate *__restrict self,
                          unsigned int value) {
 if (__BITS_OVERFLOW(VGAFORMAT_CRT_H_BLANK_END_BITS,value)) return KE_OVERFLOW;
 self->vs_crt_regs.vc_h_blank_end = (self->vs_crt_regs.vc_h_blank_end&~(VGA_CR03_H_BLANK_END))
                           |((value << VGA_CR03_H_BLANK_END_SHIFT)&
                                       VGA_CR03_H_BLANK_END);
 self->vs_crt_regs.vc_h_sync_end = (self->vs_crt_regs.vc_h_sync_end&~(VGA_CR05_H_BLANK_END_B5))
                           |(((value >> 5) << VGA_CR05_H_BLANK_END_B5_SHIFT)&
                                              VGA_CR05_H_BLANK_END_B5);
 return KE_OK;
}
__local kerrno_t
vgastate_set_crt_h_sync_end(struct vgastate *__restrict self,
                         unsigned int value) {
 if (__BITS_OVERFLOW(VGAFORMAT_CRT_H_SYNC_END_BITS,value)) return KE_OVERFLOW;
 self->vs_crt_regs.vc_h_sync_end = (self->vs_crt_regs.vc_h_sync_end&~(VGA_CR05_H_SYNC_END))
                          |((value << VGA_CR05_H_SYNC_END_SHIFT)&
                                      VGA_CR05_H_SYNC_END);
 return KE_OK;
}
__local kerrno_t
vgastate_set_crt_h_skew(struct vgastate *__restrict self,
                     unsigned int value) {
 if (__BITS_OVERFLOW(VGAFORMAT_CRT_H_SKEW_END_BITS,value)) return KE_OVERFLOW;
 self->vs_crt_regs.vc_h_blank_end = (self->vs_crt_regs.vc_h_blank_end&~(VGA_CR03_H_SKEW))
                           |((value << VGA_CR03_H_SKEW_SHIFT)&
                                       VGA_CR03_H_SKEW);
 return KE_OK;
}

__local kerrno_t
vgastate_set_crt_v_total(struct vgastate *__restrict self,
                      unsigned int value) {
 if (__BITS_OVERFLOW(VGAFORMAT_CRT_V_TOTAL_BITS,value)) return KE_OVERFLOW;
 self->vs_crt_regs.vc_v_total = value&0xff;
 self->vs_crt_regs.vc_overflow = (self->vs_crt_regs.vc_overflow&~(VGA_CR07_V_TOTAL_B8|VGA_CR07_V_TOTAL_B9))
                        |(((value >> 8) << VGA_CR07_V_TOTAL_B8_SHIFT)&
                                           VGA_CR07_V_TOTAL_B8)
                        |(((value >> 9) << VGA_CR07_V_TOTAL_B9_SHIFT)&
                                           VGA_CR07_V_TOTAL_B9);
 return KE_OK;
}
__local kerrno_t
vgastate_set_crt_v_sync_start(struct vgastate *__restrict self,
                           unsigned int value) {
 if (__BITS_OVERFLOW(VGAFORMAT_CRT_V_SYNC_START_BITS,value)) return KE_OVERFLOW;
 self->vs_crt_regs.vc_v_sync_start = value&0xff;
 self->vs_crt_regs.vc_overflow = (self->vs_crt_regs.vc_overflow&~(VGA_CR07_V_SYNC_START_B8|VGA_CR07_V_SYNC_START_B9))
                        |(((value >> 8) << VGA_CR07_V_SYNC_START_B8_SHIFT)&
                                           VGA_CR07_V_SYNC_START_B8)
                        |(((value >> 9) << VGA_CR07_V_SYNC_START_B9_SHIFT)&
                                           VGA_CR07_V_SYNC_START_B9);
 return KE_OK;
}
__local kerrno_t
vgastate_set_crt_v_sync_end(struct vgastate *__restrict self,
                         unsigned int value) {
 if (__BITS_OVERFLOW(VGAFORMAT_CRT_V_SYNC_END_BITS,value)) return KE_OVERFLOW;
 self->vs_crt_regs.vc_v_sync_end = (self->vs_crt_regs.vc_v_sync_end&~(VGA_CR11_V_SYNC_END))
                          |((value << VGA_CR11_V_SYNC_END_SHIFT)&
                                      VGA_CR11_V_SYNC_END);
 return KE_OK;
}
__local kerrno_t
vgastate_set_crt_v_disp_end(struct vgastate *__restrict self,
                         unsigned int value) {
 if (__BITS_OVERFLOW(VGAFORMAT_CRT_V_DISP_END_BITS,value)) return KE_OVERFLOW;
 self->vs_crt_regs.vc_h_disp = value&0xff;
 self->vs_crt_regs.vc_overflow = (self->vs_crt_regs.vc_overflow&~(VGA_CR07_V_DISP_END_B8|VGA_CR07_V_DISP_END_B9))
                        |(((value >> 8) << VGA_CR07_V_DISP_END_B8_SHIFT)&
                                           VGA_CR07_V_DISP_END_B8)
                        |(((value >> 9) << VGA_CR07_V_DISP_END_B9_SHIFT)&
                                           VGA_CR07_V_DISP_END_B9);
 return KE_OK;
}
__local kerrno_t
vgastate_set_crt_v_blank_start(struct vgastate *__restrict self,
                            unsigned int value) {
 if (__BITS_OVERFLOW(VGAFORMAT_CRT_V_BLANK_START_BITS,value)) return KE_OVERFLOW;
 self->vs_crt_regs.vc_h_blank_start = value&0xff;
 self->vs_crt_regs.vc_overflow = (self->vs_crt_regs.vc_overflow&~(VGA_CR07_V_BLANK_START_B8))
                        |(((value >> 8) << VGA_CR07_V_BLANK_START_B8_SHIFT)&
                                           VGA_CR07_V_BLANK_START_B8);
 self->vs_crt_regs.vc_max_scan = (self->vs_crt_regs.vc_max_scan&~(VGA_CR09_V_BLANK_START_B9))
                            |(((value >> 9) << VGA_CR09_V_BLANK_START_B9_SHIFT)&
                                               VGA_CR09_V_BLANK_START_B9);
 return KE_OK;
}


__local unsigned int
vgastate_get_crt_h_blank_end(struct vgastate const *__restrict self) {
 return ((self->vs_crt_regs.vc_h_blank_end&VGA_CR03_H_BLANK_END) >>
                                   VGA_CR03_H_BLANK_END_SHIFT)
       |(((self->vs_crt_regs.vc_h_sync_end&VGA_CR05_H_BLANK_END_B5) >>
                                   VGA_CR05_H_BLANK_END_B5_SHIFT) << 5);
}
__local unsigned int
vgastate_get_crt_h_sync_end(struct vgastate const *__restrict self) {
 return ((self->vs_crt_regs.vc_h_sync_end&VGA_CR05_H_SYNC_END) >>
                                  VGA_CR05_H_SYNC_END_SHIFT);
}
__local unsigned int
vgastate_get_crt_h_skew(struct vgastate const *__restrict self) {
 return ((self->vs_crt_regs.vc_h_blank_end&VGA_CR03_H_SKEW) >>
                                   VGA_CR03_H_SKEW_SHIFT);
}
__local unsigned int
vgastate_get_crt_v_total(struct vgastate const *__restrict self) {
 return ((unsigned int)self->vs_crt_regs.vc_v_total)
       |(((self->vs_crt_regs.vc_overflow&VGA_CR07_V_TOTAL_B8) >>
                                 VGA_CR07_V_TOTAL_B8_SHIFT) << 8)
       |(((self->vs_crt_regs.vc_overflow&VGA_CR07_V_TOTAL_B9) >>
                                 VGA_CR07_V_TOTAL_B9_SHIFT) << 9);
}
__local unsigned int
vgastate_get_crt_v_sync_start(struct vgastate const *__restrict self) {
 return ((unsigned int)self->vs_crt_regs.vc_v_sync_start)
       |(((self->vs_crt_regs.vc_overflow&VGA_CR07_V_SYNC_START_B8) >>
                                 VGA_CR07_V_SYNC_START_B8_SHIFT) << 8)
       |(((self->vs_crt_regs.vc_overflow&VGA_CR07_V_SYNC_START_B9) >>
                                 VGA_CR07_V_SYNC_START_B9_SHIFT) << 9);
}
__local unsigned int
vgastate_get_crt_v_sync_end(struct vgastate const *__restrict self) {
 return ((self->vs_crt_regs.vc_v_sync_end&VGA_CR11_V_SYNC_END) >>
                                  VGA_CR11_V_SYNC_END_SHIFT);
}
__local unsigned int
vgastate_get_crt_v_disp_end(struct vgastate const *__restrict self) {
 return ((unsigned int)self->vs_crt_regs.vc_h_disp)
       |(((self->vs_crt_regs.vc_overflow&VGA_CR07_V_DISP_END_B8) >>
                                 VGA_CR07_V_DISP_END_B8_SHIFT) << 8)
       |(((self->vs_crt_regs.vc_overflow&VGA_CR07_V_DISP_END_B9) >>
                                 VGA_CR07_V_DISP_END_B9_SHIFT) << 9);
}
__local unsigned int
vgastate_get_crt_v_blank_start(struct vgastate const *__restrict self) {
 return ((unsigned int)self->vs_crt_regs.vc_h_blank_start)
       |(((self->vs_crt_regs.vc_overflow&VGA_CR07_V_BLANK_START_B8) >>
                                 VGA_CR07_V_BLANK_START_B8_SHIFT) << 8)
       |(((self->vs_crt_regs.vc_max_scan&VGA_CR09_V_BLANK_START_B9) >>
                                     VGA_CR09_V_BLANK_START_B9_SHIFT) << 9);
}


__local __constcall vga_clock_t vgastate_get_mis_clock(struct vgastate const *__restrict self) {
 return (self->vs_mis_regs.vm_misc&VGA_MIS_CLOCK_MASK) >> VGA_MIS_CLOCK_SHIFT;
}
__local kerrno_t vgastate_set_mis_clock(struct vgastate *__restrict self, vga_clock_t value) {
 if (__BITS_OVERFLOW(VGAFORMAT_MIS_CLOCK_BITS,value)) return KE_OVERFLOW;
 self->vs_mis_regs.vm_misc = (self->vs_mis_regs.vm_misc&~(VGA_MIS_CLOCK_MASK))
                            |((value << VGA_MIS_CLOCK_SHIFT)&VGA_MIS_CLOCK_MASK);
 return KE_OK;
}
__local __constcall vga_hz_t vgastate_get_clock_hz(struct vgastate const *__restrict self) {
 static unsigned int const hz[4] = {25175000,28322000,0,0};
 return hz[vgastate_get_mis_clock(self)];
}


__local vga_bytes_t vgastate_get_pitch(struct vgastate const *__restrict self) {
 /* HINT: 'self->vs_crt_regs.vc_offset' is in bits. */
 if (self->vs_gfx_regs.vg_misc&VGA_GR06_GRAPHICS_MODE) {
  return self->vs_crt_regs.vc_offset << 3; /* /8*1 */
 } else {
  return self->vs_crt_regs.vc_offset << 2; /* /8*2 */
 }
}
__local vga_pixels_t
vgastate_get_dim_x(struct vgastate const *__restrict self) {
 unsigned int result;
 result = (vgastate_get_crt_h_disp(self)-
           vgastate_get_crt_h_skew(self))+1;
 if (self->vs_gfx_regs.vg_misc&VGA_GR06_GRAPHICS_MODE) {
  result *= vgastate_get_dots_per_char(self);
 }
 return result;
}
__local vga_pixels_t
vgastate_get_dim_y(struct vgastate const *__restrict self) {
 /* TODO: Doesn't work (at all...) */
 unsigned int result = (vgastate_get_crt_v_disp_end(self)+1);
 if (self->vs_crt_regs.vc_max_scan&VGA_CR09_DOUBLE_WIDE) result *= 2;
 return result;
 //return (vgastate_get_crt_v_disp_end(self)+1)/
 //      ((self->vs_crt_regs.vc_max_scan&0x1f)+1);
}
#endif

#undef __BITS_OVERFLOW
#undef __SAFE_DOWNCAST


/* Bochs/QEMU VBE Extensions.
 * >> https://www.virtualbox.org/svn/vbox/trunk/src/VBox/Devices/Graphics/BIOS/vbe_display_api.txt
 */
#define VBE_DISPI_BANK_ADDRESS          0xA0000
#define VBE_DISPI_BANK_SIZE_KB          64
#define VBE_DISPI_IOPORT_INDEX          0x01CE
#define VBE_DISPI_IOPORT_DATA           0x01CF
#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9
#define VBE_DISPI_ID0                   0xB0C0
#define VBE_DISPI_ID1                   0xB0C1
#define VBE_DISPI_ID2                   0xB0C2
#define VBE_DISPI_ID3                   0xB0C3
#define VBE_DISPI_ID4                   0xB0C4
#define VBE_DISPI_DISABLED              0x00 /*< for 'VBE_DISPI_INDEX_ENABLE'. */
#define VBE_DISPI_ENABLED               0x01 /*< for 'VBE_DISPI_INDEX_ENABLE'. */
#define VBE_DISPI_GETCAPS               0x02 /*< for 'VBE_DISPI_INDEX_ENABLE' - Potentially only supported by newer versions / QEMU-only... */
#define VBE_DISPI_VBE_ENABLED           0x40 /*< for 'VBE_DISPI_INDEX_ENABLE'. */
#define VBE_DISPI_NOCLEARMEM            0x80 /*< for 'VBE_DISPI_INDEX_ENABLE'. */
#define VBE_DISPI_LFB_PHYSICAL_ADDRESS  0xE0000000

__local __u16 vbe_rattr(__u16 attr);
__local void  vbe_wattr(__u16 attr, __u16 value);

struct vbeformat {
 __u16 vf_xres;
 __u16 vf_yres;
 __u16 vf_bpp;
};

#ifdef __KERNEL__
//////////////////////////////////////////////////////////////////////////
// Get/Set the VBE Display format
// NOTE: 'vbe_setformat' will fix 'self' to mirror the new device settings.
// @return: KE_OK:    Successfully read/set the format.
// @return: KE_NOSYS: The host does not support VBE.
extern __crit __nomp kerrno_t vbe_getformat(struct vbeformat *__restrict self);
extern __crit __nomp kerrno_t vbe_setformat(struct vbeformat *__restrict self);
#endif

#ifndef __INTELLISENSE__
__local __u16 vbe_rattr(__u16 attr) { outw_p(VBE_DISPI_IOPORT_INDEX,attr); return inw_p(VBE_DISPI_IOPORT_DATA); }
__local void  vbe_wattr(__u16 attr, __u16 value) { outw_p(VBE_DISPI_IOPORT_INDEX,attr); outw_p(VBE_DISPI_IOPORT_DATA,value); }
#endif



__DECL_END

#endif /* !__HW_VIDEO_VGA_H__ */
