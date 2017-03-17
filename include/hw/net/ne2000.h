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
#ifndef __HW_NET_NE2000_H__
#define __HW_NET_NE2000_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

/* Many constants and comments are extracted from qemu "/hw/net/ne2000.c".
 * The following copyright notice can be found atop that file:
 */
/*
 * QEMU NE2000 emulation
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define E8390_CMD      0x00  /* The command register (for all pages) */
/* Page 0 register offsets. */
#define EN0_CLDALO     0x01 /* Low byte of current local dma addr  RD */
#define EN0_STARTPG    0x01 /* Starting page of ring bfr WR */
#define EN0_CLDAHI     0x02 /* High byte of current local dma addr  RD */
#define EN0_STOPPG     0x02 /* Ending page +1 of ring bfr WR */
#define EN0_BOUNDARY   0x03 /* Boundary page of ring bfr RD WR */
#define EN0_TSR        0x04 /* Transmit status reg RD */
#define EN0_TPSR       0x04 /* Transmit starting page WR */
#define EN0_NCR        0x05 /* Number of collision reg RD */
#define EN0_TCNTLO     0x05 /* Low  byte of tx byte count WR */
#define EN0_FIFO       0x06 /* FIFO RD */
#define EN0_TCNTHI     0x06 /* High byte of tx byte count WR */
#define EN0_ISR        0x07 /* Interrupt status reg RD WR */
#define EN0_CRDALO     0x08 /* low byte of current remote dma address RD */
#define EN0_RSARLO     0x08 /* Remote start address reg 0 */
#define EN0_CRDAHI     0x09 /* high byte, current remote dma address RD */
#define EN0_RSARHI     0x09 /* Remote start address reg 1 */
#define EN0_RCNTLO     0x0a /* Remote byte count reg WR */
#define EN0_RTL8029ID0 0x0a /* Realtek ID byte #1 RD */
#define EN0_RCNTHI     0x0b /* Remote byte count reg WR */
#define EN0_RTL8029ID1 0x0b /* Realtek ID byte #2 RD */
#define EN0_RSR        0x0c /* rx status reg RD */
#define EN0_RXCR       0x0c /* RX configuration reg WR */
#define EN0_TXCR       0x0d /* TX configuration reg WR */
#define EN0_COUNTER0   0x0d /* Rcv alignment error counter RD */
#define EN0_DCFG       0x0e /* Data configuration reg WR */
#define EN0_COUNTER1   0x0e /* Rcv CRC error counter RD */
#define EN0_IMR        0x0f /* Interrupt mask reg WR */
#define EN0_COUNTER2   0x0f /* Rcv missed frame error counter RD */

#define EN1_PHYS       0x11
#define EN1_CURPAG     0x17
#define EN1_MULT       0x18

#define EN2_STARTPG    0x21 /* Starting page of ring bfr RD */
#define EN2_STOPPG     0x22 /* Ending page +1 of ring bfr RD */

#define EN3_CONFIG0    0x33
#define EN3_CONFIG1    0x34
#define EN3_CONFIG2    0x35
#define EN3_CONFIG3    0x36

/* Register accessed at EN_CMD, the 8390 base addr.  */
#define E8390_STOP     0x01 /* Stop and reset the chip */
#define E8390_START    0x02 /* Start the chip, clear reset */
#define E8390_TRANS    0x04 /* Transmit a frame */
#define E8390_RREAD    0x08 /* Remote read */
#define E8390_RWRITE   0x10 /* Remote write  */
#define E8390_NODMA    0x20 /* Remote DMA */
#define E8390_PAGE0    0x00 /* Select page chip registers */
#define E8390_PAGE1    0x40 /* using the two high-order bits */
#define E8390_PAGE2    0x80 /* Page 3 is invalid. */

/* Bits in EN0_ISR - Interrupt status register */
#define ENISR_RX       0x01 /* Receiver, no error */
#define ENISR_TX       0x02 /* Transmitter, no error */
#define ENISR_RX_ERR   0x04 /* Receiver, with error */
#define ENISR_TX_ERR   0x08 /* Transmitter, with error */
#define ENISR_OVER     0x10 /* Receiver overwrote the ring */
#define ENISR_COUNTERS 0x20 /* Counters need emptying */
#define ENISR_RDC      0x40 /* remote dma complete */
#define ENISR_RESET    0x80 /* Reset completed */
#define ENISR_ALL      0x3f /* Interrupts we will enable */

/* Bits in received packet status byte and EN0_RSR*/
#define ENRSR_RXOK     0x01 /* Received a good packet */
#define ENRSR_CRC      0x02 /* CRC error */
#define ENRSR_FAE      0x04 /* frame alignment error */
#define ENRSR_FO       0x08 /* FIFO overrun */
#define ENRSR_MPA      0x10 /* missed pkt */
#define ENRSR_PHY      0x20 /* physical/multicast address */
#define ENRSR_DIS      0x40 /* receiver disable. set in monitor mode */
#define ENRSR_DEF      0x80 /* deferring */

/* Transmitted packet status, EN0_TSR. */
#define ENTSR_PTX      0x01 /* Packet transmitted without error */
#define ENTSR_ND       0x02 /* The transmit wasn't deferred. */
#define ENTSR_COL      0x04 /* The transmit collided at least once. */
#define ENTSR_ABT      0x08 /* The transmit collided 16 times, and was deferred. */
#define ENTSR_CRS      0x10 /* The carrier sense was lost. */
#define ENTSR_FU       0x20 /* A "FIFO underrun" occurred during transmit. */
#define ENTSR_CDH      0x40 /* The collision detect "heartbeat" signal was lost. */
#define ENTSR_OWC      0x80 /* There was an out-of-window collision. */





__DECL_END

#endif /* !__HW_NET_NE2000_H__ */
