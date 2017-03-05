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
#ifndef __KOS_KERNEL_PTY_C__
#define __KOS_KERNEL_PTY_C__ 1

#include <kos/arch.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/vfs-dev.h>
#include <kos/kernel/fs/vfs.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/pty.h>
#include <kos/kernel/util/string.h>
#include <kos/syslog.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

__DECL_BEGIN

#if 1 /* Full-blocking write (May cause slave apps to hang if the driver crashes) */
#define PTY_WRITE_BLOCKING_MODE   KIO_BLOCKALL
#else
#define PTY_WRITE_BLOCKING_MODE   KIO_BLOCKNONE
#endif


void kpty_init(struct kpty *__restrict self,
               struct termios const *__restrict ios,
               struct winsize const *__restrict size) {
 kassertobj(self);
 kobject_init(self,KOBJECT_MAGIC_PTY);
 self->ty_num = (kptynum_t)-1;
 kiobuf_init_ex(&self->ty_s2m,PTY_IO_BUFFER_SIZE);
 kiobuf_init_ex(&self->ty_m2s,PTY_IO_BUFFER_SIZE);
 krwlock_init(&self->ty_lock);
 krawbuf_init_ex(&self->ty_canon,PTY_LINE_BUFFER_SIZE);
 if (size) memcpy(&self->ty_size,size,sizeof(struct winsize));
 else {
#ifdef VGA_HEIGHT
  self->ty_size.ws_row = VGA_HEIGHT;
#else
  self->ty_size.ws_row = 25;
#endif
#ifdef VGA_WIDTH
  self->ty_size.ws_col = VGA_WIDTH;
#else
  self->ty_size.ws_col = 80;
#endif
  self->ty_size.ws_xpixel = self->ty_size.ws_row;
  self->ty_size.ws_ypixel = self->ty_size.ws_col;
 }
 self->ty_cproc = NULL;
 self->ty_fproc = NULL;
 if (ios) memcpy(&self->ty_ios,ios,sizeof(struct termios));
 else {
  memset(&self->ty_ios,0,sizeof(struct termios));
  self->ty_ios.c_iflag      = ICRNL|BRKINT;
  self->ty_ios.c_oflag      = /*ONLCR|*/OPOST;
  self->ty_ios.c_lflag      = ECHO|ECHOE|ECHOK|ICANON|ISIG|IEXTEN;
  self->ty_ios.c_cflag      = CREAD;
  self->ty_ios.c_cc[VMIN]   =  1; /* Just use KIO_BLOCKFIRST-style behavior by default. */
  self->ty_ios.c_cc[VEOF]   =  4; /* ^D. */
  self->ty_ios.c_cc[VEOL]   =  0; /* Not set. */
  self->ty_ios.c_cc[VERASE] = '\b';
  self->ty_ios.c_cc[VINTR]  =  3; /* ^C. */
  self->ty_ios.c_cc[VKILL]  = 21; /* ^U. */
  self->ty_ios.c_cc[VQUIT]  = 28; /* ^\. */
  self->ty_ios.c_cc[VSTART] = 17; /* ^Q. */
  self->ty_ios.c_cc[VSTOP]  = 19; /* ^S. */
  self->ty_ios.c_cc[VSUSP]  = 26; /* ^Z. */
  self->ty_ios.c_cc[VTIME]  =  0;
 }
}

__crit void kpty_quit(struct kpty *__restrict self) {
 KTASK_CRIT_MARK
 kassert_kpty(self);
 kiobuf_quit(&self->ty_s2m);
 kiobuf_quit(&self->ty_m2s);
 /* KTASK_NOINTR_BEGIN */
 __evalexpr(krwlock_close(&self->ty_lock));
 /* KTASK_NOINTR_END */
 krawbuf_quit(&self->ty_canon);
 if (self->ty_cproc) kproc_decref(self->ty_cproc);
 if (self->ty_fproc) kproc_decref(self->ty_fproc);
}


__local __crit kerrno_t
kpty_dumpcanon_c(struct kpty *__restrict self) {
 kerrno_t error; size_t wsize,canbufsize; void *canbuf;
 KTASK_CRIT_MARK
 KTASK_NOINTR_BEGIN
 krawbuf_capture_begin(&self->ty_canon);
 canbuf     = krawbuf_capture_data(&self->ty_canon);
 canbufsize = krawbuf_capture_size(&self->ty_canon);
 krawbuf_capture_end_inherit(&self->ty_canon);
 error = kiobuf_write(&self->ty_m2s,canbuf,canbufsize,
                      &wsize,PTY_WRITE_BLOCKING_MODE);
 free(canbuf);
 KTASK_NOINTR_END
 return error;
}

kerrno_t
kpty_user_ioctl(struct kpty *__restrict self,
                kattr_t cmd, __user void *arg) {
 kerrno_t error;
 kassert_kpty(self);
 KTASK_CRIT_BEGIN
 switch (cmd) {

  case TCSAFLUSH:
   switch ((int)arg) {
    case TCIOFLUSH:
    case TCIFLUSH:
     /* Discard data received, but not read (aka. what the driver send) */
     kiobuf_discard(&self->ty_m2s);
    case TCOFLUSH:
     if ((int)arg != TCIFLUSH) {
      /* Discard data written, but not send (aka. what the slave told to the driver). */
      kiobuf_discard(&self->ty_s2m);
     }
     error = KE_OK;
     break;
    default: error = KE_INVAL; break;
   }
   break;

  case TIOCGWINSZ:
   if __likely(KE_ISOK(error = krwlock_beginread(&self->ty_lock))) {
    error = copy_to_user(arg,&self->ty_size,sizeof(self->ty_size)) ? KE_FAULT : KE_OK;
    krwlock_endread(&self->ty_lock);
   }
   break;

  {
   struct winsize new_winsize;
  case TIOCSWINSZ:
   if __unlikely(copy_from_user(&new_winsize,arg,sizeof(new_winsize))) goto err_fault;
   if __likely(KE_ISOK(error = krwlock_beginwrite(&self->ty_lock))) {
    memcpy(&self->ty_size,&new_winsize,sizeof(struct winsize));
    /* TODO: Send SIGWINCH to ty_fproc */
    krwlock_endwrite(&self->ty_lock);
   }
   break;
  }

  case TCGETS:
   if __likely(KE_ISOK(error = krwlock_beginread(&self->ty_lock))) {
    error = copy_to_user(arg,&self->ty_ios,sizeof(self->ty_ios)) ? KE_FAULT : KE_OK;
    krwlock_endread(&self->ty_lock);
   }
   break;

  {
   struct termios new_ios;
  case TCSETS:
  case TCSETSW:
  case TCSETSF:
   if __unlikely(copy_from_user(&new_ios,arg,sizeof(new_ios))) goto err_fault;
   if __likely(KE_ISOK(error = krwlock_beginwrite(&self->ty_lock))) {
    if (!(new_ios.c_lflag & ICANON) && (self->ty_ios.c_lflag & ICANON)) {
     /* When switching out of line-buffered mode, first dump the canon. */
     error = kpty_dumpcanon_c(self);
     if __unlikely(KE_ISERR(error)) goto end_setios;
    }
    memcpy(&self->ty_ios,&new_ios,sizeof(struct termios));
end_setios:
    krwlock_endwrite(&self->ty_lock);
   }
   break;
  }

  {
   pid_t pid;
   struct kproc *newproc,*oldproc;
  case TIOCSPGRP:
   if __unlikely(copy_from_user(&pid,arg,sizeof(pid))) goto err_fault;
   newproc = (struct kproc *)kproclist_getproc(pid);
   /* NOTE: To not undermine the idea of the GETPROP/VISIBILITY barrier,
    *       we don't differentiate between invisible and invalid PIDs. */
   if __unlikely(!newproc) { error = KE_PERM; goto end; }
   if __likely(KE_ISOK(error = krwlock_beginwrite(&self->ty_lock))) {
    oldproc = self->ty_fproc; /*< Inherit reference */
    self->ty_fproc = newproc; /*< Inherit reference */
    k_syslogf(KLOG_INFO,"[PTY] Set group %I32d (%p)\n",pid,newproc);
    krwlock_endwrite(&self->ty_lock);
    if (oldproc) kproc_decref(oldproc);
   }
   break;
  }

  {
   pid_t respid;
  case TIOCGPGRP:
   if __likely(KE_ISOK(error = krwlock_beginread(&self->ty_lock))) {
    respid = self->ty_fproc ? self->ty_fproc->p_pid : PID_MAX;
    krwlock_endread(&self->ty_lock);
    if __unlikely(copy_to_user(arg,&respid,sizeof(respid))) error = KE_FAULT;
   }
   break;
  }

  default:
   error = KE_NOSYS;
   break;
 }
end:
 KTASK_CRIT_END
 return error;
err_fault: error = KE_FAULT; goto end;
}


__local kerrno_t
kpty_s2m_write_crlf_unlocked(struct kpty *__restrict self,
                             __kernel void const *buf, size_t bufsize,
                             __kernel size_t *wsize, kioflag_t mode) {
 char const *iter,*end,*flush_start;
 kerrno_t error; size_t partsize,didpartsize;
 *wsize = 0;
 /* Must convert '\n' to '\r\n' (LF -> CRLF; 10 -> 13,10) */
 end = (flush_start = iter = (char const *)buf)+(bufsize/sizeof(char));
 for (;;) {
  if (iter == end || *iter == '\n') {
   static char const crlf[] = {'\r','\n'};
   partsize = (size_t)(iter-flush_start)*sizeof(char);
   error = kiobuf_write(&self->ty_s2m,flush_start,partsize,
                        &didpartsize,mode);
   if (iter == end || KE_ISERR(error) || didpartsize != partsize) break;
   error = kiobuf_write(&self->ty_s2m,crlf,sizeof(crlf),
                        &partsize,mode);
   if __unlikely(KE_ISERR(error)) break;
   *wsize += didpartsize;
   if (partsize != sizeof(sizeof(crlf))) break;
   ++*wsize;
   flush_start = iter+1;
  }
  ++iter;
 }
 return error;
}
__local kerrno_t
kpty_s2m_write_unlocked(struct kpty *__restrict self,
                        __kernel void const *buf, size_t bufsize,
                        __kernel size_t *wsize, kioflag_t mode) {
 kassert_kpty(self);
 assert(krwlock_isreadlocked(&self->ty_lock));
 if (!(self->ty_ios.c_oflag&ONLCR)) {
  return kiobuf_write(&self->ty_s2m,buf,bufsize,wsize,mode);
 }
 return kpty_s2m_write_crlf_unlocked(self,buf,bufsize,wsize,mode);
}
__local kerrno_t
kpty_s2m_user_write_unlocked(struct kpty *__restrict self,
                             __user void const *buf, size_t bufsize,
                             __kernel size_t *wsize) {
 __kernel void *part_p;
 size_t part_s,temp; kerrno_t error = KE_OK;
 kassert_kpty(self);
 assert(krwlock_isreadlocked(&self->ty_lock));
 if (!(self->ty_ios.c_oflag&ONLCR)) {
  return kiobuf_user_write(&self->ty_s2m,buf,bufsize,
                           wsize,PTY_WRITE_BLOCKING_MODE);
 }
 *wsize = 0;
 USER_FOREACH_BEGIN(buf,bufsize,part_p,part_s,0) {
  error = kpty_s2m_write_crlf_unlocked(self,part_p,part_s,
                                       &temp,KIO_BLOCKNONE);
  if __unlikely(KE_ISERR(error)) USER_FOREACH_BREAK;
  *wsize += temp;
 } USER_FOREACH_END({
  return KE_FAULT;
 });
 return error;
}

static char const ERASE[] = {'\b',' ','\b'};
kerrno_t kpty_m_special_canon(struct kpty *__restrict self, __u8 ch) {
 kerrno_t error; size_t temp;
 kassert_kpty(self);
 if (ch == self->ty_ios.c_cc[VKILL]) {
  size_t capture_size;
  /* Clear input line and ERASE echo text */
  KTASK_CRIT_BEGIN
  krawbuf_capture_begin(&self->ty_canon);
  capture_size = krawbuf_capture_size(&self->ty_canon);
  krawbuf_capture_end(&self->ty_canon);
  KTASK_CRIT_END
  if (self->ty_ios.c_lflag & ECHO) {
   char *text,*iter,*end; size_t text_size;
   text_size = sizeof(ERASE)*capture_size;
   if __likely(capture_size > 1 && (text = (char *)malloc(text_size)) != NULL) {
    end = (iter = text)+(text_size/sizeof(char));
    for (; iter != end; iter += (sizeof(ERASE)/sizeof(char)))
     memcpy(iter,ERASE,sizeof(ERASE));
    error = kiobuf_write(&self->ty_s2m,text,text_size,&temp,PTY_WRITE_BLOCKING_MODE);
    free(text);
   } else while (capture_size--) {
    error = kiobuf_write(&self->ty_s2m,ERASE,sizeof(ERASE),&temp,PTY_WRITE_BLOCKING_MODE);
    if __unlikely(KE_ISERR(error)) break;
   }
  } else {
   return KE_OK;
  }
  goto ret_error_nosignal;
 } else if (ch == self->ty_ios.c_cc[VERASE]) {
  int did_erase;
  /* Erase single character from canon */
  KTASK_CRIT_BEGIN
  krawbuf_lock(&self->ty_canon,KRAWBUF_LOCK_DATA);
  did_erase = (self->ty_canon.rb_bufpos != self->ty_canon.rb_buffer);
  if (did_erase) --self->ty_canon.rb_bufpos;
  krawbuf_unlock(&self->ty_canon,KRAWBUF_LOCK_DATA);
  KTASK_CRIT_END
  /* Send erase sequence to pty mater */
  return (did_erase && (self->ty_ios.c_lflag & ECHO))
          ? kiobuf_write(&self->ty_s2m,ERASE,sizeof(ERASE),
                         &temp,PTY_WRITE_BLOCKING_MODE)
          : KE_OK;
 } else if (ch == self->ty_ios.c_cc[VINTR] ||
            ch == self->ty_ios.c_cc[VQUIT]) {
  if (self->ty_ios.c_lflag & ECHO) {
   char out_text[3] = {'^','@'+ch,'\n'};
   error = kpty_s2m_write_unlocked(self,out_text,sizeof(out_text),&temp,
                                   PTY_WRITE_BLOCKING_MODE);
   if __unlikely(KE_ISERR(error)) return error;
  }
  /* TODO: Send SIGINT/SIGQUIT to 'self->ty_fproc' instead of just terminating it... */
  if (self->ty_fproc) {
   struct ktask *root_task;
   KTASK_CRIT_BEGIN
   root_task = kproc_getroottask(self->ty_fproc);
   if __likely(root_task) {
    error = ktask_terminateex(root_task,NULL,KTASKOPFLAG_TREE|KTASKOPFLAG_ASYNC);
    ktask_decref(root_task);
   } else error = KE_OK;
   KTASK_CRIT_END
   goto ret_error_nosignal;
  }
  return KE_OK;
 } else if (ch == self->ty_ios.c_cc[VEOF]) {
  KTASK_CRIT_BEGIN
  krawbuf_capture_begin(&self->ty_canon);
  /* Flush the canon, or interrupt a block-first read
   * operation originating from the pty slave process. */
  if (krawbuf_capture_size(&self->ty_canon)) {
   void  *canbuf     = krawbuf_capture_data(&self->ty_canon);
   size_t canbufsize = krawbuf_capture_size(&self->ty_canon);
   krawbuf_capture_end_inherit(&self->ty_canon);
   error = kiobuf_write(&self->ty_m2s,canbuf,canbufsize,
                        &temp,PTY_WRITE_BLOCKING_MODE);
   free(canbuf);
  } else {
   krawbuf_capture_end(&self->ty_canon);
   error = kiobuf_interrupt(&self->ty_m2s);
  }
  KTASK_CRIT_END
ret_error_nosignal:
   return KE_ISERR(error) ? error : KE_OK;
 }
 return KS_UNCHANGED;
}


kerrno_t
kpty_mwrite_canon_unlocked(struct kpty *__restrict self, __kernel void const *buf,
                           size_t bufsize, __kernel size_t *__restrict wsize, kioflag_t mode) {
 kerrno_t error; size_t temp;
 cc_t const *iter,*end,*canon_start,*new_canon_start;
 kassert_kpty(self);
 assert(krwlock_isreadlocked(&self->ty_lock));
 /* Must use the line buffer, as well
  * as check for control characters */
 end = (canon_start = iter = (cc_t const *)buf)+bufsize;
again:
 for (;;) {
  if (iter == end) {
   size_t canon_size;
flush_canon:
   canon_size = (size_t)(iter-canon_start)*sizeof(cc_t);
   error = krawbuf_write(&self->ty_canon,canon_start,canon_size,&temp);
   if __unlikely(KE_ISERR(error)) return error;
#if 0
   if __unlikely(temp != canon_size) { bufsize = (iter-(cc_t const *)buf)*sizeof(cc_t); goto end; }
#endif
   if (self->ty_ios.c_lflag&ECHO) {
    error = kpty_s2m_write_unlocked(self,canon_start,canon_size,&temp,mode);
    if __unlikely(KE_ISERR(error)) return error;
   }
   if (new_canon_start == iter) {
    size_t canbufsize; void *canbuf;
    assert(iter != (cc_t const *)buf);
    /* Can't assert because user may have changed it:
     * assertf(iter[-1] == '\n',"'%c' %I8x",iter[-1],iter[-1]); */
    /* Write the canon to the slave */
    KTASK_CRIT_BEGIN
    krawbuf_capture_begin(&self->ty_canon);
#if 0
    printf("FLUSH: %Iu %.?q\n"
          ,krawbuf_capture_size(&self->ty_canon)
          ,krawbuf_capture_size(&self->ty_canon)
          ,krawbuf_capture_data(&self->ty_canon));
#endif
    canbuf     = krawbuf_capture_data(&self->ty_canon);
    canbufsize = krawbuf_capture_size(&self->ty_canon);
    krawbuf_capture_end_inherit(&self->ty_canon);
    error = kiobuf_write(&self->ty_m2s,canbuf,canbufsize,
                         &temp,mode);
    free(canbuf);
    KTASK_CRIT_END
    if __unlikely(KE_ISERR(error)) return error;
    if (iter == end) break;
    goto again;
   }
   if (iter == end || ++iter == end) break;
   canon_start = new_canon_start;
  } else if (*(char *)iter == '\n') {
   new_canon_start = ++iter;
   goto flush_canon;
  } else {
   error = kpty_m_special_canon(self,*iter);
   if __unlikely(error != KS_UNCHANGED) {
    if __unlikely(KE_ISERR(error)) return error;
    assertf(error == KE_OK,"Unexpected signal: %d",error);
    new_canon_start = iter+1;
    goto flush_canon;
   }
   ++iter;
  }
 }
/*end:*/
 *wsize = bufsize;
 return KE_OK;
}

kerrno_t
kpty_mwrite_unlocked(struct kpty *__restrict self, __kernel void const *buf,
                     size_t bufsize, __kernel size_t *__restrict wsize) {
 kerrno_t error; size_t temp;
 kassert_kpty(self);
 assert(krwlock_isreadlocked(&self->ty_lock));
 if (!(self->ty_ios.c_lflag&ICANON)) {
  if (self->ty_ios.c_lflag&ECHO) {
   error = kpty_s2m_write_unlocked(self,buf,bufsize,&temp,
                                   PTY_WRITE_BLOCKING_MODE);
   if __unlikely(KE_ISERR(error)) return error;
  }
  return kiobuf_write(&self->ty_m2s,buf,bufsize,
                      wsize,PTY_WRITE_BLOCKING_MODE);
 }
 return kpty_mwrite_canon_unlocked(self,buf,bufsize,wsize,
                                   PTY_WRITE_BLOCKING_MODE);
}

kerrno_t
kpty_user_mwrite_unlocked(struct kpty *__restrict self, __user void const *buf,
                          size_t bufsize, __kernel size_t *__restrict wsize) {
 kerrno_t error; size_t temp;
 __kernel void *part_p; size_t part_s;
 kassert_kpty(self);
 assert(krwlock_isreadlocked(&self->ty_lock));
 if (!(self->ty_ios.c_lflag&ICANON)) {
  if (self->ty_ios.c_lflag&ECHO) {
   error = kpty_s2m_user_write_unlocked(self,buf,bufsize,&temp);
   if __unlikely(KE_ISERR(error)) return error;
  }
  return kiobuf_user_write(&self->ty_m2s,buf,bufsize,
                           wsize,PTY_WRITE_BLOCKING_MODE);
 }
 *wsize = 0;
 USER_FOREACH_BEGIN(buf,bufsize,part_p,part_s,0) {
  error = kpty_mwrite_canon_unlocked(self,part_p,part_s,
                                     &temp,KIO_BLOCKNONE);
  if __unlikely(KE_ISERR(error)) USER_FOREACH_BREAK;
  *wsize += temp;
  if (temp != part_s) USER_FOREACH_BREAK;
 } USER_FOREACH_END({
  return KE_FAULT;
 });
 return error;
}

kerrno_t
kpty_mwrite(struct kpty *__restrict self, __kernel void const *buf,
            size_t bufsize, __kernel size_t *__restrict wsize) {
 kerrno_t error;
 kassert_kpty(self);
 if __likely(KE_ISOK(error = krwlock_beginread(&self->ty_lock))) {
  error = kpty_mwrite_unlocked(self,buf,bufsize,wsize);
  krwlock_endread(&self->ty_lock);
 }
 return error;
}
kerrno_t
kpty_user_mwrite(struct kpty *__restrict self, __user void const *buf,
                 size_t bufsize, __kernel size_t *__restrict wsize) {
 kerrno_t error;
 kassert_kpty(self);
 if __likely(KE_ISOK(error = krwlock_beginread(&self->ty_lock))) {
  error = kpty_user_mwrite_unlocked(self,buf,bufsize,wsize);
  krwlock_endread(&self->ty_lock);
 }
 return error;
}


kerrno_t kpty_user_sread(struct kpty *__restrict self, __user void *buf,
                         size_t bufsize, size_t *__restrict rsize) {
 kerrno_t error; kioflag_t iomode; cc_t min_read;
 kassert_kpty(self);
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = krwlock_beginread(&self->ty_lock))) {
  /* In canonical mode, block until the first byte has been read */
  if (self->ty_ios.c_lflag&ICANON) iomode = KIO_BLOCKFIRST;
  else if ((min_read = self->ty_ios.c_cc[VMIN]) == 0) {
   /* Non-blocking read, when data is available */
   iomode = KIO_BLOCKNONE;
  } else {
   size_t part;
   /* Blocking read, until at least 'min_read' characters have been read */
   krwlock_endread(&self->ty_lock);
   *rsize = 0; do {
    error = kiobuf_user_read(&self->ty_m2s,buf,bufsize,&part,KIO_BLOCKFIRST);
    /* Shouldn't happen, but might due to race conditions... (I/O interrupt) */
    if __unlikely(!part) break;
    *rsize += part;
   } while (*rsize < min_read);
   goto end;
  }
  krwlock_endread(&self->ty_lock);
  error = kiobuf_user_read(&self->ty_m2s,buf,bufsize,rsize,iomode);
 }
end:
 KTASK_CRIT_END
 return error;
}

kerrno_t kpty_swrite(struct kpty *__restrict self,
                     __kernel void const *buf, size_t bufsize,
                     __kernel size_t *__restrict wsize) {
 kerrno_t error;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = krwlock_beginread(&self->ty_lock))) {
  error = kpty_s2m_write_unlocked(self,buf,bufsize,wsize,
                                  PTY_WRITE_BLOCKING_MODE);
  krwlock_endread(&self->ty_lock);
 }
 KTASK_CRIT_END
 return error;
}

kerrno_t kpty_user_swrite(struct kpty *__restrict self,
                          __user void const *buf, size_t bufsize,
                          __kernel size_t *__restrict wsize) {
 kerrno_t error;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = krwlock_beginread(&self->ty_lock))) {
  error = kpty_s2m_user_write_unlocked(self,buf,bufsize,wsize);
  krwlock_endread(&self->ty_lock);
 }
 KTASK_CRIT_END
 return error;
}


kerrno_t
kfspty_user_mgetattr(struct kfspty const *self,
                size_t ac, __user union kinodeattr av[]) {
 kerrno_t error;
 union kinodeattr attr;
 for (; ac; --ac,++av) {
  if __unlikely(copy_from_user(&attr,av,sizeof(union kinodeattr))) return KE_FAULT;
  switch (attr.ia_common.a_id) {
   case KATTR_FS_PERM:
    attr.ia_perm.p_perm = S_IRUSR|S_IWUSR|S_IRGRP;
    break;
   {
    size_t maxsize;
   case KATTR_FS_SIZE:
    error = kiobuf_getrsize(&self->fp_pty.ty_s2m,&maxsize);
    if __unlikely(KE_ISERR(error)) return error;
    attr.ia_size.sz_size = maxsize;
   } break;
   default:
    error = kinode_user_generic_getattr((struct kinode *)self,1,av);
    if __unlikely(KE_ISERR(error)) return error;
    goto next_attr;
  }
  if __unlikely(copy_to_user(av,&attr,sizeof(union kinodeattr))) return KE_FAULT;
next_attr:;
 }
 return KE_OK;
}
kerrno_t
kfspty_user_sgetattr(struct kfspty const *self, size_t ac,
                __user union kinodeattr av[]) {
 kerrno_t error;
 union kinodeattr attr;
 for (; ac; --ac,++av) {
  if __unlikely(copy_from_user(&attr,av,sizeof(union kinodeattr))) return KE_FAULT;
  switch (attr.ia_common.a_id) {
   case KATTR_FS_PERM:
    attr.ia_perm.p_perm = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP;
    break;
   {
    size_t maxsize;
   case KATTR_FS_SIZE:
    error = kiobuf_getrsize(&self->fp_pty.ty_m2s,&maxsize);
    if __unlikely(KE_ISERR(error)) return error;
    attr.ia_size.sz_size = maxsize;
   } break;
   default:
    error = kinode_user_generic_getattr((struct kinode *)self,1,av);
    if __unlikely(KE_ISERR(error)) return error;
    goto next_attr;
  }
  if __unlikely(copy_to_user(av,&attr,sizeof(union kinodeattr))) return KE_FAULT;
next_attr:;
 }
 return KE_OK;
}



//////////////////////////////////////////////////////////////////////////
// File system integration from here on...

__crit __ref struct kfspty *
kfspty_new(struct termios const *__restrict ios,
           struct winsize const *__restrict size) {
 __ref struct kfspty *result;
 KTASK_CRIT_MARK
 result = (__ref struct kfspty *)__kinode_alloc((struct ksuperblock *)&kvfs_dev,
                                                &kfspty_type,
                                                &kptyfile_master_type,
                                                S_IFCHR);
 if __unlikely(!result) return NULL;
 kpty_init(&result->fp_pty,ios,size);
 return result;
}

static __atomic kptynum_t next_pty_num = 0;


struct kfspty_slave_inode {
       struct kinode  s_node; /*< Underlying INode. */
 __ref struct kfspty *s_pty;  /*< [1..1] Associated PTY Master INode. */
};
static void kfspty_slave_inode_quit(struct kinode *__restrict self) {
 kinode_decref(&((struct kfspty_slave_inode *)self)->s_pty->fp_node);
}

struct kinodetype kfspty_slave_type = {
 .it_size    = sizeof(struct kfspty_slave_inode),
 .it_quit    = &kfspty_slave_inode_quit,
 .it_getattr = (kerrno_t(*)(struct kinode const *,size_t,union kinodeattr *))&kfspty_user_sgetattr,
};


static void kfspty_quit(struct kinode *__restrict self) {
 kpty_quit(&((struct kfspty *)self)->fp_pty);
}

struct kinodetype kfspty_type = {
 .it_size    = sizeof(struct kfspty),
 .it_quit    = &kfspty_quit,
 .it_getattr = (kerrno_t(*)(struct kinode const *,size_t,union kinodeattr *))&kfspty_user_mgetattr,
};




__crit kerrno_t
kfspty_insnod(struct kfspty *__restrict self,
              __ref struct kdirent **__restrict master_ent,
              __ref struct kdirent **__restrict slave_ent,
              __user char *master_name_buf) {
 kerrno_t error; kptynum_t resnum;
 char master_name[32],slave_name[32];
 struct kproc *caller = kproc_self();
 struct kfspathenv env;
 size_t master_name_size,slave_name_size;
 __ref struct kfspty_slave_inode *slave_inode;
 KTASK_CRIT_MARK
 kassert_kinode(&self->fp_node);
 kassert_kpty(&self->fp_pty);
 kassertobj(master_ent);
 kassertobj(slave_ent);
 /* Allocate the slave INode. */
 error = kinode_incref((struct kinode *)self);
 if __unlikely(KE_ISERR(error)) return error;
 slave_inode = (__ref struct kfspty_slave_inode *)
  __kinode_alloc((struct ksuperblock *)&kvfs_dev,
                 &kfspty_slave_type,
                 &kptyfile_slave_type,
                 S_IFCHR);
 if __unlikely(!slave_inode) {
  kinode_decref((struct kinode *)self);
  return KE_NOMEM;
 }
 slave_inode->s_pty = self; /* Inherit reference. */
 do {
  resnum = katomic_load(next_pty_num);
  if __unlikely(resnum == (kptynum_t)-1) {
   /* TODO: Manually search for free slot */
   error = KE_OVERFLOW;
   goto err_slave;
  }
 } while (!katomic_cmpxch(next_pty_num,resnum,resnum+1));
 if (!katomic_cmpxch(self->fp_pty.ty_num,(kptynum_t)-1,resnum)) {
  error = KE_EXISTS;
  goto err_gnum;
 }
 /* Setup a filesystem context to use for resolving paths.
  * Don't use a root context, to still consider chroot-prisons!
  */
 env.env_root = kproc_getfddirent(caller,KFD_ROOT);
 if __unlikely(!env.env_root) { error = KE_NOROOT; goto err_num; }
 env.env_cwd = env.env_root;
 env.env_flags = 0;
 env.env_uid   = kproc_getuid(caller);
 env.env_gid   = kproc_getgid(caller);
 kfspathenv_initcommon(&env);
 /* Generate the file names for the master and slave directory entries. */
 master_name_size = sprintf(master_name,"/dev/tty%I32u",resnum);
 slave_name_size  = sprintf(slave_name,"/dev/ttyS%I32u",resnum);
 if (master_name_buf &&
     copy_to_user(master_name_buf,master_name,master_name_size*sizeof(char))
     ) { error = KE_FAULT; goto err_root; }
 /* Actually Insert the nodes! */
 error = kdirent_insnodat(&env,master_name,master_name_size,
                         (struct kinode *)self,master_ent);
 if __unlikely(KE_ISERR(error)) goto err_root;
 error = kdirent_insnodat(&env,slave_name,slave_name_size,
                         (struct kinode *)slave_inode,slave_ent);
 if __unlikely(KE_ISERR(error)) goto err_ment;
 k_syslogf(KLOG_INFO,"[PTY] Registered PTY device (num: %I32u; master: %.?q; slave: %.?q)\n",
           resnum,master_name_size,master_name,slave_name_size,slave_name);
 /* YAY! We've successfully created all the nodes. - Time for some cleanup. */
 kinode_decref((struct kinode *)slave_inode);
 kdirent_decref(env.env_root);
 return error;
err_ment:  kdirent_decref(*master_ent);
err_root:  kdirent_decref(env.env_root);
err_num:   katomic_cmpxch(self->fp_pty.ty_num,resnum,(kptynum_t)-1);
err_gnum:  katomic_cmpxch(next_pty_num,resnum+1,resnum);
err_slave: kinode_decref((struct kinode *)slave_inode);
 return error;
}


__local __crit __nonnull((1,3)) __ref struct kptyfile *
kptyfile_new(struct kfspty *__restrict pty,
             struct kdirent *dent,
             struct kfiletype *__restrict type) {
 __ref struct kptyfile *result;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kinode_incref(&pty->fp_node))) return NULL;
 if __unlikely(dent && KE_ISERR(kdirent_incref(dent))) {
  kinode_decref(&pty->fp_node);
  return NULL;
 }
 if __unlikely((result = omalloc(__ref struct kptyfile)) == NULL) return NULL;
 kfile_init(&result->pf_file,type);
 result->pf_pty = pty; /* Inherit reference. */
 result->pf_dent = dent; /* Inherit reference. */
 return result;
}

__crit __ref struct kptyfile *
kptyfile_slave_new(struct kfspty *__restrict pty,
                   struct kdirent *dent) {
 KTASK_CRIT_MARK
 return kptyfile_new(pty,dent,&kptyfile_slave_type);
}
__crit __ref struct kptyfile *
kptyfile_master_new(struct kfspty *__restrict pty,
                    struct kdirent *dent) {
 KTASK_CRIT_MARK
 return kptyfile_new(pty,dent,&kptyfile_master_type);
}


#define SELF  ((struct kptyfile *)self)
static void kptyfile_quit(struct kfile *__restrict self) {
 if (SELF->pf_dent) kdirent_decref(SELF->pf_dent);
 kinode_decref(&SELF->pf_pty->fp_node);
}
static kerrno_t
kptyfile_open(struct kfile *__restrict self, struct kdirent *__restrict dirent,
              struct kinode *__restrict inode, openmode_t mode) {
 kerrno_t error;
 /* Resolve slave-node proxy reference */
 if (inode->i_type == &kfspty_slave_type) {
  inode = (struct kinode *)((struct kfspty_slave_inode *)inode)->s_pty;
 }
 assert(inode->i_type == &kfspty_type);
 if __unlikely(KE_ISERR(error = kinode_incref(inode))) return error;
 if __unlikely(KE_ISERR(error = kdirent_incref(SELF->pf_dent = dirent)))
 { kinode_decref(inode); return error; }
 SELF->pf_pty = (__ref struct kfspty *)inode;
 return error;
}

static __ref struct kinode *kptyfile_getinode(struct kfile *__restrict self) {
 __ref struct kinode *result = (struct kinode *)SELF->pf_pty;
 if __unlikely(KE_ISERR(kinode_incref(result))) result = NULL;
 return result;
}
static __ref struct kdirent *kptyfile_getdirent(struct kfile *__restrict self) {
 __ref struct kdirent *result = SELF->pf_dent;
 if __unlikely(KE_ISERR(kdirent_incref(result))) result = NULL;
 return result;
}


static kerrno_t
kptyfile_master_read(struct kfile *__restrict self,
                     __user void *__restrict buf, size_t bufsize,
                     __kernel size_t *__restrict rsize) {
 return kpty_user_mread(&SELF->pf_pty->fp_pty,buf,bufsize,rsize);
}
static kerrno_t
kptyfile_master_write(struct kfile *__restrict self,
                      __user void const *__restrict buf, size_t bufsize,
                      __kernel size_t *__restrict wsize) {
 return kpty_user_mwrite(&SELF->pf_pty->fp_pty,buf,bufsize,wsize);
}
static kerrno_t
kptyfile_slave_read(struct kfile *__restrict self,
                    __user void *__restrict buf, size_t bufsize,
                    __kernel size_t *__restrict rsize) {
 return kpty_user_sread(&SELF->pf_pty->fp_pty,buf,bufsize,rsize);
}
static kerrno_t
kptyfile_slave_write(struct kfile *__restrict self,
                     __user void const *__restrict buf, size_t bufsize,
                     __kernel size_t *__restrict wsize) {
 return kpty_user_swrite(&SELF->pf_pty->fp_pty,buf,bufsize,wsize);
}
static kerrno_t
kptyfile_ioctl(struct kfile *__restrict self,
               kattr_t cmd, __user void *arg) {
 return kpty_user_ioctl(&SELF->pf_pty->fp_pty,cmd,arg);
}
#undef SELF

struct kfiletype kptyfile_slave_type = {
 .ft_size      = sizeof(struct kptyfile),
 .ft_quit      = &kptyfile_quit,
 .ft_open      = &kptyfile_open,
 .ft_read      = &kptyfile_slave_read,
 .ft_write     = &kptyfile_slave_write,
 .ft_ioctl     = &kptyfile_ioctl,
 .ft_getinode  = &kptyfile_getinode,
 .ft_getdirent = &kptyfile_getdirent,
};
struct kfiletype kptyfile_master_type = {
 .ft_size      = sizeof(struct kptyfile),
 .ft_quit      = &kptyfile_quit,
 .ft_open      = &kptyfile_open,
 .ft_read      = &kptyfile_master_read,
 .ft_write     = &kptyfile_master_write,
 .ft_ioctl     = &kptyfile_ioctl,
 .ft_getinode  = &kptyfile_getinode,
 .ft_getdirent = &kptyfile_getdirent,
};



__DECL_END

#endif /* !__KOS_KERNEL_PTY_C__ */
