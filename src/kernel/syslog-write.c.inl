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
#include "syslog.c"
#define USER
__DECL_BEGIN
#endif

#ifndef __syslog_last_writer_defined
#define __syslog_last_writer_defined 1
static __atomic int  syslog_lock = 0;
static struct kproc *syslog_last_writer = NULL;
#endif

#ifndef SIZEOF_STRING
#define SIZEOF_STRING(x) (sizeof(x)/sizeof(char)-1)
#endif

#ifdef USER
kerrno_t k_dosyslog_u(struct kproc *caller, int level, 
                      __user char const *s, size_t maxlen)
#else
void k_dosyslog(int level, void (*print_prefix)(int,void *),
                void *closure, __kernel char const *s, size_t maxlen)
#define caller  kproc_kernel()
#endif
{
 char const *flush_start,*iter,*end,*mnemonic
#if !KLOGFORMAT_PREFIXTIME
   = NULL
#endif
  ;
#if KLOGFORMAT_PREFIXTIME
 char *tmbuf = NULL;
#endif
 __u32 new_state,old_state;
#ifdef USER
 char *appname,*kernel_s;
 size_t partmaxsize; kerrno_t error;
 kassert_kproc(caller);
#endif

#ifdef USER
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = kproc_lock(caller,KPROC_LOCK_SHM))) {
  for (;;) {
   partmaxsize = maxlen;
   kernel_s = (char *)kshm_translate_u(&caller->p_shm,s,&partmaxsize,0);
   if __unlikely(!kernel_s) { error = KE_FAULT; break; /* FAULT */ }
   assert(partmaxsize <= maxlen);
#define s      kernel_s
#define maxlen partmaxsize
#endif /* USER */
   end = (flush_start = iter = s)+maxlen;
   for (;;) {
    if (iter == end || !*iter || *iter++ == '\n') {
     int haslf = (iter != s && iter[-1] == '\n');
     new_state = old_state = caller->p_sand.ts_state;
     if (!haslf) new_state |=  (KPROCSTATE_FLAG_SYSLOGINL);
     else        new_state &= ~(KPROCSTATE_FLAG_SYSLOGINL);
     caller->p_sand.ts_state = new_state;
     /* Begin logging. */
     while (!katomic_cmpxch(syslog_lock,0,1)) ktask_yield();
     if (syslog_last_writer != caller) {
      if __likely(syslog_last_writer) {
       /* Special handling for interleaved logging.
          When multiple tasks log concurrently,
          still always prepend their prefix! */
       if (syslog_last_writer->p_sand.ts_state&KPROCSTATE_FLAG_SYSLOGINL
           ) k_writesyslog(level,"\n",1);
       syslog_last_writer = caller;
       goto do_print_prefix;
      } else {
       syslog_last_writer = caller;
      }
     }
     if (!(old_state&KPROCSTATE_FLAG_SYSLOGINL)) {
do_print_prefix:
#if KLOGFORMAT_PREFIXTIME
      if (!tmbuf)
#else
      if (!mnemonic)
#endif
      {
#if KLOGFORMAT_PREFIXTIME
       struct timespec tmnow; ktime_getnoworcpu(&tmnow);
#endif
#ifdef USER
       struct kshlib *root_lib = kproc_getrootexe(caller);
       appname = (char *)alloca(PATH_MAX*sizeof(char));
#endif
#if KLOGFORMAT_PREFIXTIME
       tmbuf = (char *)alloca(32*sizeof(char));
#endif
#ifdef USER
       if __unlikely(!root_lib) {
err_appname: strcpy(appname,"??" "?");
       } else {
        if (KE_ISERR(kfile_getattr(root_lib->sh_file,KATTR_FS_PATHNAME,appname,PATH_MAX*sizeof(char),NULL)) &&
            KE_ISERR(kfile_getattr(root_lib->sh_file,KATTR_FS_FILENAME,appname,PATH_MAX*sizeof(char),NULL))
            ) goto err_appname;
        kshlib_decref(root_lib);
       }
       appname[PATH_MAX-1] = '\0';
#endif
#if KLOGFORMAT_PREFIXTIME
       sprintf(tmbuf,KLOGFORMAT_TAGSTR_1 "%.6I32u.%.6ld"
               KLOGFORMAT_TAGSTR_2 KLOGFORMAT_TAGSTR_1,
               tmnow.tv_sec,tmnow.tv_nsec);
#endif
       mnemonic = k_sysloglevel_mnemonic(level);
      }
#if KLOGFORMAT_PREFIXTIME
      k_writesyslog(level,tmbuf,32);
#else
      k_writesyslog(level,KLOGFORMAT_TAGSTR_1,SIZEOF_STRING(KLOGFORMAT_TAGSTR_1));
#endif
      k_writesyslog(level,mnemonic,(size_t)-1);
#ifdef USER
      if (caller == kproc_kernel())
#endif
      {
#ifndef USER
       if (print_prefix) {
        k_writesyslog(level,KLOGFORMAT_TAGSTR_2 KLOGFORMAT_KERNEL,
                      SIZEOF_STRING(KLOGFORMAT_TAGSTR_2)+
                      SIZEOF_STRING(KLOGFORMAT_KERNEL));
        (*print_prefix)(level,closure);
        if (*flush_start != KLOGFORMAT_TAGCH_1) k_writesyslog(level," ",1);
       } else
#endif
       {
        k_writesyslog(level,KLOGFORMAT_TAGSTR_2 KLOGFORMAT_KERNEL " ",
                      *flush_start == KLOGFORMAT_TAGCH_1
                      ? SIZEOF_STRING(KLOGFORMAT_TAGSTR_2)+SIZEOF_STRING(KLOGFORMAT_KERNEL)
                      : SIZEOF_STRING(KLOGFORMAT_TAGSTR_2)+SIZEOF_STRING(KLOGFORMAT_KERNEL)+1);
       }
      }
#ifdef USER
      else {
       k_writesyslog(level,KLOGFORMAT_TAGSTR_2 KLOGFORMAT_APP_1,
                     SIZEOF_STRING(KLOGFORMAT_TAGSTR_2)+
                     SIZEOF_STRING(KLOGFORMAT_APP_1));
       k_writesyslog(level,appname,PATH_MAX);
       k_writesyslog(level,KLOGFORMAT_APP_2 " ",
                     *flush_start == KLOGFORMAT_TAGCH_1
                     ? SIZEOF_STRING(KLOGFORMAT_APP_2)
                     : SIZEOF_STRING(KLOGFORMAT_APP_2)+1);
      }
#endif
     }
     k_writesyslog(level,flush_start,(size_t)(iter-flush_start));
     /* End logging. */
     katomic_store(syslog_lock,0);
#ifdef USER
     if (iter == end) break;
     if (!*iter) goto done;
#else
     if (iter == end || !*iter) break;
#endif
     flush_start = iter+1;
    }
   }
#ifdef USER
#undef maxlen
#undef s
   if (partmaxsize == maxlen) break;
   s      += partmaxsize;
   maxlen -= partmaxsize;
  }
done:
  kproc_unlock(caller,KPROC_LOCK_SHM);
 }
 KTASK_CRIT_END
#endif /* USER */
#ifdef USER
 return error;
#else
#undef caller
#endif
}

#ifdef USER
#undef USER
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif
