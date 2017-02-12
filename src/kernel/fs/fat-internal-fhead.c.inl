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
#ifndef __KOS_KERNEL_FS_FAT_INTERNAL_FHEAD_C_INL__
#define __KOS_KERNEL_FS_FAT_INTERNAL_FHEAD_C_INL__ 1

#include <alloca.h>
#include <assert.h>
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/kernel/time.h>
#include <kos/kernel/types.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <kos/syslog.h>

__DECL_BEGIN

__local time_t kfatfile_date_totimet(struct kfatfile_date const *self) {
 time_t result; unsigned int year;
 year = self->fd_year+1980;
 result = __time_years2days(year-__LINUX_TIME_START_YEAR);
 result += __time_monthstart_yday[__time_isleapyear(year)][(self->fd_month-1) % 12];
 result += (self->fd_day-1);
 return result*__SECS_PER_DAY;
}
__local int kfatfile_date_fromtimet(struct kfatfile_date *self, time_t tmt) {
 struct kfatfile_date newdate;
 __u8 i; time_t const *monthvec; unsigned int year;
 tmt /= __SECS_PER_DAY;
 tmt += __time_years2days(__LINUX_TIME_START_YEAR);
 year = __time_days2years(tmt);
 monthvec = __time_monthstart_yday[__time_isleapyear(year)];
 tmt -= __time_years2days(year);
 newdate.fd_year = year-1980;
 // Find the appropriate month
 for (i = 1; i < 12; ++i) if (monthvec[i] >= tmt) break;
 newdate.fd_month = i;
 newdate.fd_day = (tmt-monthvec[i-1])+1;
 if (self->fd_hash == newdate.fd_hash) return 0;
 *self = newdate;
 return 1;
}
__local time_t kfatfile_time_totimet(struct kfatfile_time const *self) {
 return ((time_t)self->ft_hour*60*60)+
        ((time_t)self->ft_min*60)+
        ((time_t)self->ft_sec*2);
}
__local int kfatfile_time_fromtimet(struct kfatfile_time *self, time_t tmt) {
 struct kfatfile_time newtime;
 newtime.ft_sec  = (tmt % 60)/2;
 newtime.ft_min  = ((tmt/60) % 60);
 newtime.ft_hour = ((tmt/(60*60)) % 24);
 if (self->fd_hash == newtime.fd_hash) return 0;
 *self = newtime;
 return 1;
}

void kfatfileheader_getatime(struct kfatfileheader const *self, struct timespec *result) {
 kassertobj(self); kassertobj(result);
 result->tv_sec = kfatfile_date_totimet(&self->f_atime);
 result->tv_nsec = 0;
}
void kfatfileheader_getctime(struct kfatfileheader const *self, struct timespec *result) {
 kassertobj(self); kassertobj(result);
 result->tv_sec  = kfatfile_date_totimet(&self->f_ctime.fc_date);
 result->tv_sec += kfatfile_time_totimet(&self->f_ctime.fc_time);
 result->tv_nsec = (long)self->f_ctime.fc_sectenth*(1000000000l/200l);
}
void kfatfileheader_getmtime(struct kfatfileheader const *self, struct timespec *result) {
 kassertobj(self); kassertobj(result);
 result->tv_sec  = kfatfile_date_totimet(&self->f_mtime.fc_date);
 result->tv_sec += kfatfile_time_totimet(&self->f_mtime.fc_time);
 result->tv_nsec = 0;
}
int kfatfileheader_setatime(struct kfatfileheader *self, struct timespec const *value) {
 kassertobj(self); kassertobj(value);
 return kfatfile_date_fromtimet(&self->f_atime,value->tv_sec);
}
int kfatfileheader_setctime(struct kfatfileheader *self, struct timespec const *value) {
 int changed; __u8 newnth;
 kassertobj(self); kassertobj(value);
 changed = kfatfile_date_fromtimet(&self->f_ctime.fc_date,value->tv_sec)
        || kfatfile_time_fromtimet(&self->f_ctime.fc_time,value->tv_sec);
 newnth = (__u8)((value->tv_nsec % 1000000000l)/(1000000000l/200l));
 if (self->f_ctime.fc_sectenth != newnth) {
  self->f_ctime.fc_sectenth = newnth;
  changed = 1;
 }
 return changed;
}
int kfatfileheader_setmtime(struct kfatfileheader *self, struct timespec const *value) {
 kassertobj(self); kassertobj(value);
 return kfatfile_date_fromtimet(&self->f_mtime.fc_date,value->tv_sec)
     || kfatfile_time_fromtimet(&self->f_mtime.fc_time,value->tv_sec);
}


kerrno_t kfatfs_savefileheader(struct kfatfs *self, __u64 headpos,
                               struct kfatfileheader const *header) {
 kfatsec_t headsec; void *secbuf; kerrno_t error;
 struct kfatfileheader *headbuf;
 kassertobj(self); kassertobj(header);
 headsec = (kfatsec_t)(headpos/self->f_secsize);
 secbuf = alloca(self->f_secsize);
 // Load the sector that contains the file's header
 error = kfatfs_loadsectors(self,headsec,1,secbuf);
 if __unlikely(KE_ISERR(error)) return error;
 headbuf = (struct kfatfileheader *)((uintptr_t)secbuf+(size_t)(headpos % self->f_secsize));
 assertf(headbuf+1 <= (struct kfatfileheader *)((uintptr_t)secbuf+self->f_secsize),
         "File header spans multiple sectors.");
 k_syslogf(KLOG_TRACE,"Updating FAT file header @ %I64u: '%.8s.%.3s'\n",
           headpos,headbuf->f_name,headbuf->f_ext);
 memcpy(headbuf,header,sizeof(struct kfatfileheader));
 // Save the sector again
 return kfatfs_savesectors(self,headsec,1,secbuf);
}

kerrno_t kfatfs_delheader(struct kfatfs *self, __u64 headpos) {
 kfatsec_t headsec; void *secbuf; kerrno_t error;
 struct kfatfileheader *headbuf;
 kassertobj(self); kassertobj(header);
 headsec = (kfatsec_t)(headpos/self->f_secsize);
 secbuf = alloca(self->f_secsize);
 /* Load the sector that contains the file's header */
 error = kfatfs_loadsectors(self,headsec,1,secbuf);
 if __unlikely(KE_ISERR(error)) return error;
 headbuf = (struct kfatfileheader *)((uintptr_t)secbuf+(size_t)(headpos % self->f_secsize));
 assertf(headbuf+1 <= (struct kfatfileheader *)((uintptr_t)secbuf+self->f_secsize),
         "File header spans multiple sectors.");
 k_syslogf(KLOG_TRACE,"Updating FAT file header @ %I64u: '%.8s.%.3s'\n",
           headpos,headbuf->f_name,headbuf->f_ext);
 headbuf->f_marker = KFATFILE_MARKER_UNUSED;
 /* Save the sector again */
 return kfatfs_savesectors(self,headsec,1,secbuf);
}


__DECL_END

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_FHEAD_C_INL__ */
