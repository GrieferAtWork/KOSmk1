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
#ifndef __KOS_KERNEL_DEV_STORAGE_ATA_C_INL__
#define __KOS_KERNEL_DEV_STORAGE_ATA_C_INL__ 1

#include <assert.h>
#include <kos/compiler.h>
#include <kos/kernel/dev/storage-ata.h>
#include <kos/errno.h>
#include <kos/kernel/task.h>
#include <kos/syslog.h>
#include <sys/io.h>
#include <kos/kernel/debug.h>
#include <byteswap.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <kos/endian.h>
#include <sys/types.h>
#include <kos/kernel/sched_yield.h>

__DECL_BEGIN

void kata_sleep(katabus_t bus) {
 // As recommended here: http://wiki.osdev.org/ATA_PIO_Mode
 __evalexpr(inb(ATA_IOPORT_ALTSTATUS(bus)));
 __evalexpr(inb(ATA_IOPORT_ALTSTATUS(bus)));
 __evalexpr(inb(ATA_IOPORT_ALTSTATUS(bus)));
 __evalexpr(inb(ATA_IOPORT_ALTSTATUS(bus)));
}

kerrno_t kata_poll(katabus_t bus, __u8 mask, __u8 state) {
 __u8 status;
 //kata_sleep(bus);
 do {
  status = inb(ATA_IOPORT_STATUS(bus));
  if (status & ATA_STATUS_ERR) {
   k_syslogf(KLOG_ERROR,"[DISK][ATA] ERR received while polling (flags = 0x%I8x)\n",status);
   return KE_DEVICE;
  }
 } while ((status & mask) != state);
 return KE_OK;
}


#ifndef __INTELLISENSE__
__STATIC_ASSERT(sizeof(struct ata_identifydata) == 512);
#endif

static kerrno_t __katadev_pio28_readlba(katadev_t const *dev, kslba_t lba, void *__restrict buf, size_t sectors, size_t *readsectors);
static kerrno_t __katadev_pio28_writelba(katadev_t *dev, kslba_t lba, void const *__restrict buf, size_t sectors, size_t *writesectors);
static kerrno_t __katadev_pio48_readlba(katadev_t const *dev, kslba_t lba, void *__restrict buf, size_t sectors, size_t *readsectors);
static kerrno_t __katadev_pio48_writelba(katadev_t *dev, kslba_t lba, void const *__restrict buf, size_t sectors, size_t *writesectors);


static kerrno_t katadev_getname(
 katadev_t const *self, void *__restrict buf,
 size_t bufsize, size_t *__restrict reqsize) {
 char mybuf[40]; char *bufbegin,*bufend;
 __u16 *dst,*src,*end; size_t namesize;
 kassertobj(self);
 end = (src = (__u16 *)self->ad_info.ModelNumber)+20;
 dst = (__u16 *)mybuf;
#if __BYTE_ORDER == __LITTLE_ENDIAN
 while (src != end) *dst++ = bswap_16(*src++);
#else
 memcpy(dst,src,(end-src)*sizeof(__u16));
#endif
 bufend = mybuf+40;
 while (bufend != mybuf && isspace(bufend[-1])) --bufend;
 bufbegin = mybuf;
 while (bufbegin != bufend && isspace(bufbegin[0])) ++bufbegin;
 namesize = (size_t)(bufend-bufbegin);
 memcpy(buf,bufbegin,(namesize < bufsize ? namesize : bufsize)*sizeof(char));
 if (namesize < bufsize) ((char *)buf)[namesize] = '\0';
 if (reqsize) *reqsize = (namesize+1)*sizeof(char);
 return KE_OK;
}

static kerrno_t katadev_getattr(
 katadev_t const *self, kattr_t attr,
 void *__restrict buf, size_t bufsize, size_t *__restrict reqsize) {
 switch (attr) {
  case KATTR_GENERIC_NAME:
  case KATTR_DEV_NAME:
   return katadev_getname(self,buf,bufsize,reqsize);
  case KATTR_DEV_TYPE: {
   size_t usedsize = snprintf((char *)buf,bufsize,"ATA");
   if (reqsize) *reqsize = usedsize;
   return KE_OK;
  } break;


  case KATTR_DEV_PARTITIONS:
   return ksdev_generic_getdiskinfo512((struct ksdev *)self,(struct ksdev_diskinfo *)buf,bufsize,reqsize);
  case KATTR_DEV_MAXDISKNAME:
   if (reqsize) *reqsize = sizeof(size_t);
   if (bufsize >= sizeof(size_t)) *(size_t *)buf = KSDEV_GENERIC_MAXDISKNAMESIZE512;
   return KE_OK;
  case KATTR_DEV_DISKNAME:
   return ksdev_generic_getdiskname512((struct ksdev *)self,(char *)buf,bufsize,reqsize);
 }
 return KE_NOSYS;
}

static kerrno_t katadev_setattr(
 katadev_t const *self, kattr_t attr,
 void *__restrict buf, size_t bufsize, size_t *__restrict reqsize) {
 switch (attr) {
  case KATTR_DEV_PARTITIONS:
   return ksdev_generic_setdiskinfo512((struct ksdev *)self,(struct ksdev_diskinfo *)buf,bufsize);
  case KATTR_DEV_DISKNAME:
   return ksdev_generic_setdiskname512((struct ksdev *)self,(char *)buf,bufsize);
  default: break;
 }
 return KE_NOSYS;
}


kerrno_t katadev_create(katadev_t *self, katabus_t bus, katadrive_t drive) {
 __u8 status; __u16 *iter,*end;
 kassertobj(self);
 outb(ATA_IOPORT_HDDEVSEL(bus), drive);
 outb(ATA_IOPORT_SECCOUNT0(bus),0);
 outb(ATA_IOPORT_LBALO(bus),    0);
 outb(ATA_IOPORT_LBAMD(bus),    0);
 outb(ATA_IOPORT_LBAHI(bus),    0);
 outb(ATA_IOPORT_COMMAND(bus),  ATA_CMD_IDENTIFY);
 status = inb(ATA_IOPORT_STATUS(bus));
 if (!status) return KE_NOSYS; // No response: not a valid device
 k_syslogf(KLOG_INFO,"[DISK][ATA] Found device on %I16x|%I8x\n",bus,drive);
 // Wait until the busy flag goes away
 while (status & ATA_STATUS_BSY) ktask_yield(),status = inb(ATA_IOPORT_STATUS(bus));
 // Wait for the DRQ signal
 while ((status & (ATA_STATUS_ERR|ATA_STATUS_DRQ)) == 0)
  ktask_yield(),status = inb(ATA_IOPORT_STATUS(bus));
 if (status&ATA_STATUS_ERR) return KE_DEVICE;
 // Everything's OK. - Now just read the device information
 end = (iter = (__u16 *)&self->ad_info)+256;
 while (iter != end) *iter++ = inw(ATA_IOPORT_DATA(bus));

 // Select most appropriate read/write operators
 self->ad_sdev.sd_blocksize = KATA_SECTORSIZE;
 kdev_init(&self->ad_sdev);
 if (self->ad_info.CommandSetActive.BigLba) {
  k_syslogf(KLOG_INFO,"[DISK][ATA] Device on %I16x|%I8x supports LBA48\n",bus,drive);
  self->ad_sdev.sd_readblocks  = (kerrno_t(*)(struct ksdev const *,kslba_t,void *,size_t,size_t *))&__katadev_pio48_readlba;
  self->ad_sdev.sd_writeblocks = (kerrno_t(*)(struct ksdev *,kslba_t,void const *,size_t,size_t *))&__katadev_pio48_writelba;
  self->ad_sdev.sd_blockcount  = self->ad_info.UserAddressableSectors48;
 } else {
  self->ad_sdev.sd_readblocks  = (kerrno_t(*)(struct ksdev const *,kslba_t,void *,size_t,size_t *))&__katadev_pio28_readlba;
  self->ad_sdev.sd_writeblocks = (kerrno_t(*)(struct ksdev *,kslba_t,void const *,size_t,size_t *))&__katadev_pio28_writelba;
  self->ad_sdev.sd_blockcount  = (kslba_t)self->ad_info.UserAddressableSectors;
 }
 self->ad_sdev.sd_readblock  = &ksdev_generic_readblock;
 self->ad_sdev.sd_writeblock = &ksdev_generic_writeblock;
 self->ad_sdev.d_quit = NULL;
 self->ad_sdev.d_getattr = (kerrno_t(*)(struct kdev const *,kattr_t,void *,size_t,size_t *))&katadev_getattr;
 self->ad_sdev.d_setattr = (kerrno_t(*)(struct kdev *,kattr_t,void const *,size_t))&katadev_setattr;
 self->ad_bus   = bus;
 self->ad_drive = drive;
 return KE_OK;
}

#ifndef __INTELLISENSE__
#define LBA48
#include "storage-ata-rwlba.c.inl"
#include "storage-ata-rwlba.c.inl"
#endif

__DECL_END

#endif /* !__KOS_KERNEL_DEV_STORAGE_ATA_C_INL__ */
