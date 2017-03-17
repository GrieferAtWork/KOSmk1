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
#ifndef __KOS_KERNEL_VGA_C__
#define __KOS_KERNEL_VGA_C__ 1

#include <assert.h>
#include <hw/video/vga.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <math.h>
#include <string.h>
#include <sys/io.h>

__DECL_BEGIN

void kernel_initialize_vga(void) {
 /* Make sure ports '0x3D4' and '0x3DA' are mapped correctly. */
 __u8 v = inb_p(VGA_MIS_R);
 if (!(v&VGA_MIS_COLOR)) outb(VGA_MIS_W,v|VGA_MIS_COLOR);

 //VBE_DISPI_LFB_PHYSICAL_ADDRESS;
}


void vgastate_save(struct vgastate *__restrict self) {
 int i;
 kassertobj(self);
 for (i = 0; i < VGA_CRT_C; ++i) self->vs_crt[i] = vga_rcrt(i);
 for (i = 0; i < VGA_ATT_C; ++i) self->vs_att[i] = vga_rattr(i);
 for (i = 0; i < VGA_GFX_C; ++i) self->vs_gfx[i] = vga_rgfx(i);
 for (i = 0; i < VGA_SEQ_C; ++i) self->vs_seq[i] = vga_rseq(i);
 self->vs_mis[0] = vga_io_r(VGA_ATT_R);
#if defined(__DEBUG__) && 1 /* Test the get/set function pairs for correctness. */
#define ASSERT_GETSET(get,set) \
 do{ unsigned int nv,v = get(self);\
     assert(KE_ISOK(set(self,v)));\
     nv = get(self);\
     assertf(nv == v,"%u != %u",nv,v);\
 }while(0)
 ASSERT_GETSET(vgastate_get_crt_h_total,      vgastate_set_crt_h_total);
 ASSERT_GETSET(vgastate_get_crt_h_disp,   vgastate_set_crt_h_disp);
 ASSERT_GETSET(vgastate_get_crt_h_blank_start,vgastate_set_crt_h_blank_start);
 ASSERT_GETSET(vgastate_get_crt_h_sync_start, vgastate_set_crt_h_sync_start);
 ASSERT_GETSET(vgastate_get_crt_h_blank_end,  vgastate_set_crt_h_blank_end);
 ASSERT_GETSET(vgastate_get_crt_h_sync_end,   vgastate_set_crt_h_sync_end);
 ASSERT_GETSET(vgastate_get_crt_h_skew,       vgastate_set_crt_h_skew);
 ASSERT_GETSET(vgastate_get_crt_v_total,      vgastate_set_crt_v_total);
 ASSERT_GETSET(vgastate_get_crt_v_sync_start, vgastate_set_crt_v_sync_start);
 ASSERT_GETSET(vgastate_get_crt_v_sync_end,   vgastate_set_crt_v_sync_end);
 ASSERT_GETSET(vgastate_get_crt_v_disp_end,   vgastate_set_crt_v_disp_end);
 ASSERT_GETSET(vgastate_get_crt_v_blank_start,vgastate_set_crt_v_blank_start);
 ASSERT_GETSET(vgastate_get_crt_v_blank_end,  vgastate_set_crt_v_blank_end);
#undef ASSERT_GETSET
#endif
 vga_io_r(VGA_IS1_RC);
 outb(VGA_ATT_IW,0x20); /*< TODO: What exactly is this doing? */
}

void vgastate_restore(struct vgastate const *__restrict self) {
 int i;
 kassertobj(self);
/* Disable write protection. */
 vga_wcrt(VGA_CRTC_V_SYNC_END,self->vs_crt_regs.vc_v_sync_end&~(VGA_CR11_LOCK_CR0_CR7));
 vga_io_w(VGA_ATT_R,self->vs_mis[0]);
 for (i = VGA_SEQ_C; i-- != 0;) vga_wseq(i,self->vs_seq[i]);
 for (i = VGA_GFX_C; i-- != 0;) vga_wgfx(i,self->vs_gfx[i]);
 for (i = VGA_ATT_C; i-- != 0;) vga_wattr(i,self->vs_att[i]);
 for (i = VGA_CRT_C; i-- != 0;) if (i != VGA_CRTC_V_SYNC_END) vga_wcrt(i,self->vs_crt[i]);
 if (self->vs_crt_regs.vc_v_sync_end&VGA_CR11_LOCK_CR0_CR7) {
  vga_wcrt(VGA_CRTC_V_SYNC_END,self->vs_crt_regs.vc_v_sync_end);
 }
 vga_io_r(VGA_IS1_RC);
 outb(VGA_ATT_IW,0x20); /*< TODO: What exactly is this doing? */
}



__crit __nomp kerrno_t
vbe_getformat(struct vbeformat *__restrict self) {
 kerrno_t result = KE_OK;
 kassertobj(self);
 self->vf_xres = vbe_rattr(VBE_DISPI_INDEX_XRES);
 self->vf_yres = vbe_rattr(VBE_DISPI_INDEX_YRES);
 self->vf_bpp  = vbe_rattr(VBE_DISPI_INDEX_BPP);
 if (!self->vf_xres ||
     !self->vf_yres ||
     !self->vf_bpp) result = KE_NOSYS;
 return result;
}

__crit __nomp kerrno_t
vbe_setformat(struct vbeformat *__restrict self) {
 kassertobj(self);
 vbe_wattr(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_DISABLED);
 vbe_wattr(VBE_DISPI_INDEX_XRES,self->vf_xres);
 vbe_wattr(VBE_DISPI_INDEX_YRES,self->vf_yres);
 vbe_wattr(VBE_DISPI_INDEX_BPP, self->vf_bpp);
 vbe_wattr(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_VBE_ENABLED|VBE_DISPI_ENABLED);
 return vbe_getformat(self);
}



// void vsync() {
//   /* wait until a running vsync has ended. */
//   while (inb(VGA_IS1_RC)&VGA_IS1_V_RETRACE);
//   /* wait until a new vsync has started. */
//   while (!(inb(VGA_IS1_RC)&VGA_IS1_V_RETRACE));
// }



__DECL_END

#endif /* !__KOS_KERNEL_VGA_H__ */
