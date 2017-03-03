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
#ifdef __INTELLISENSE__
#include "storage-ata.c.inl"
__DECL_BEGIN
#define LBA48
#endif

#ifdef LBA48
#define MAX_SECTORS  0xffff
#else
#define MAX_SECTORS  0xff
#endif


#ifdef LBA48
static kerrno_t __katadev_pio48_readlba
#else
static kerrno_t __katadev_pio28_readlba
#endif
(struct katadev const *__restrict self, kslba_t lba,
 __kernel void *__restrict buf, size_t sectors,
 __kernel size_t *readsectors) {
 kerrno_t error = KE_OK; katabus_t bus;
 __u16 *iter;
#ifdef LBA48
 typedef __u16 Tseci;
#else
 typedef __u8 Tseci;
#endif
 Tseci seci;
 kassertobj(self);
 kassertobj(readsectors);
 kassertmem(buf,(sectors*KATA_SECTORSIZE));
 assertf(lba < self->sd_blockcount
        ,"Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n"
        ,lba,self->sd_blockcount);
 if (sectors > MAX_SECTORS) sectors = MAX_SECTORS;
 bus = self->ad_bus;
 NOIRQ_BEGIN;
 if __unlikely(KE_ISERR(error = kata_poll(bus,ATA_STATUS_BSY,0))) goto end;
#ifdef LBA48
 outb(ATA_IOPORT_HDDEVSEL(bus),
     ((self->ad_drive == ATA_DRIVE_SLAVE) ? 0x50 : 0x40));
#else
 outb(ATA_IOPORT_HDDEVSEL(bus),
     ((self->ad_drive == ATA_DRIVE_SLAVE) ? 0xF0 : 0xE0) |
     ((lba >> 24) & 0x0F));
#endif
 if __unlikely(KE_ISERR(error = kata_poll(bus,
  ATA_STATUS_BSY|ATA_STATUS_DRDY,ATA_STATUS_DRDY))) goto end;
 //kata_sleep(bus);
#ifdef LBA48
 outb(ATA_IOPORT_SECCOUNT0(bus),(__u8)((Tseci)sectors >> 8));
 outb(ATA_IOPORT_LBALO(bus),(__u8)(lba >> 24));
 outb(ATA_IOPORT_LBAMD(bus),(__u8)(lba >> 32));
 outb(ATA_IOPORT_LBAHI(bus),(__u8)(lba >> 40));
#endif
 outb(ATA_IOPORT_SECCOUNT0(bus),(__u8)(Tseci)sectors);
 outb(ATA_IOPORT_LBALO(bus),(__u8)lba);
 outb(ATA_IOPORT_LBAMD(bus),(__u8)(lba >> 8));
 outb(ATA_IOPORT_LBAHI(bus),(__u8)(lba >> 16));
#ifdef LBA48
 outb(ATA_IOPORT_COMMAND(bus),ATA_CMD_READ_PIO_EXT);
#else
 outb(ATA_IOPORT_COMMAND(bus),ATA_CMD_READ_PIO);
#endif
 if __unlikely(KE_ISERR(error = kata_poll(bus,ATA_STATUS_DRQ,ATA_STATUS_DRQ))) goto end;
 iter = (__u16 *)buf;
 for (seci = 0; seci != (Tseci)sectors; ++seci) {
  insw(ATA_IOPORT_DATA(bus),iter,KATA_SECTORSIZE/sizeof(__u16));
  iter += KATA_SECTORSIZE/sizeof(__u16);
 }
 *readsectors = (size_t)seci;
 kata_sleep(bus);
end:
 NOIRQ_END
 return error;
}


#ifdef LBA48
static kerrno_t __katadev_pio48_writelba
#else
static kerrno_t __katadev_pio28_writelba
#endif
(struct katadev *__restrict self, kslba_t lba,
 __kernel void const *__restrict buf, size_t sectors,
 __kernel size_t *writesectors) {
#if 0
 *writesectors = sectors;
 return KE_OK;
#else
 kerrno_t error = KE_OK; katabus_t bus;
 __u16 *iter,*end;
#ifdef LBA48
 typedef __u16 Tseci;
#else
 typedef __u8 Tseci;
#endif
 Tseci seci;
 kassertobj(self);
 kassertobj(writesectors);
 kassertmem(buf,(sectors*KATA_SECTORSIZE));
 assertf(lba < self->sd_blockcount
        ,"Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n"
        ,lba,self->sd_blockcount);
 if (sectors > MAX_SECTORS) sectors = MAX_SECTORS;
 bus = self->ad_bus;
 NOIRQ_BEGIN;
 if __unlikely(KE_ISERR(error = kata_poll(bus,ATA_STATUS_BSY,0))) goto end;
#ifdef LBA48
 outb(ATA_IOPORT_HDDEVSEL(bus),
      ((self->ad_drive == ATA_DRIVE_SLAVE) ? 0x50 : 0x40));
#else
 outb(ATA_IOPORT_HDDEVSEL(bus),
      ((self->ad_drive == ATA_DRIVE_SLAVE) ? 0xF0 : 0xE0) |
      ((lba >> 24) & 0x0F));
#endif
 if __unlikely(KE_ISERR(error = kata_poll(bus,
  ATA_STATUS_BSY|ATA_STATUS_DRDY,ATA_STATUS_DRDY))) goto end;
 //outb(ATA_IOPORT_ERROR(bus),0);
#ifdef LBA48
 outb(ATA_IOPORT_SECCOUNT0(bus),(__u8)((Tseci)sectors >> 8));
 outb(ATA_IOPORT_LBALO(bus),(__u8)(lba >> 24));
 outb(ATA_IOPORT_LBAMD(bus),(__u8)(lba >> 32));
 outb(ATA_IOPORT_LBAHI(bus),(__u8)(lba >> 40));
#endif
 outb(ATA_IOPORT_SECCOUNT0(bus),(__u8)(Tseci)sectors);
 outb(ATA_IOPORT_LBALO(bus),(__u8)lba);
 outb(ATA_IOPORT_LBAMD(bus),(__u8)(lba >> 8));
 outb(ATA_IOPORT_LBAHI(bus),(__u8)(lba >> 16));
#ifdef LBA48
 outb(ATA_IOPORT_COMMAND(bus),ATA_CMD_WRITE_PIO_EXT);
#else
 outb(ATA_IOPORT_COMMAND(bus),ATA_CMD_WRITE_PIO);
#endif
 iter = (__u16 *)buf;
 for (seci = 0; seci != (Tseci)sectors; ++seci) {
  if __unlikely(KE_ISERR(error = kata_poll(bus,ATA_STATUS_DRQ,ATA_STATUS_DRQ))) break;
  end = iter+(KATA_SECTORSIZE/sizeof(__u16));
  while (iter != end) outw(ATA_IOPORT_DATA(bus),*iter++);
 }
#ifdef LBA48
 outb(ATA_IOPORT_COMMAND(bus),ATA_CMD_CACHE_FLUSH_EXT);
 kata_sleep(bus);
#endif
 outb(ATA_IOPORT_COMMAND(bus),ATA_CMD_CACHE_FLUSH);
 kata_sleep(bus);
 *writesectors = (size_t)seci;
end:
 NOIRQ_END
 return error;
#endif
}

#undef MAX_SECTORS

#ifdef LBA48
#undef LBA48
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif
