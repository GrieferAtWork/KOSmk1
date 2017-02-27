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
#ifndef __STDLIB_C__
#define __STDLIB_C__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <lib/libc.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __KERNEL__
#include <kos/arch/asm.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/tty.h>
#else
#include <assert.h>
#include <mod.h>
#include <errno.h>
#include <kos/atomic.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <proc.h>
#endif

#ifndef KDEBUG_SOURCEPATH_PREFIX
#define KDEBUG_SOURCEPATH_PREFIX "../"
#endif

__DECL_BEGIN

#ifdef __KERNEL__
#define ABORT(exitcode) arch_hang()
#else
#define ABORT(exitcode) kproc_exit(exitcode)
#endif

#ifdef __HAVE_ATEXIT
static void __runatexit(int exitcode);
#else
#define __runatexit(exitcode) (void)0
#endif

__public void __Exit(__uintptr_t exitcode) { ABORT((void *)exitcode); }
__public void _exit(int exitcode) { ABORT((void *)exitcode); }
__public void exit(int exitcode) { __runatexit(exitcode); ABORT((void *)exitcode); } // TODO: Run atexit

#ifndef __CONFIG_MIN_LIBC__
#undef abs
#undef labs
__public int abs(int n) { return n < 0 ? -n : n; }
__public long labs(long n) { return n < 0 ? -n : n; }
#ifndef __NO_longlong
#undef llabs
__public long long llabs(long long n) { return n < 0 ? -n : n; }
#endif /* !__NO_longlong */
#endif /* !__CONFIG_MIN_LIBC__ */


#undef abort
__public void abort(void) {
#if defined(__DEBUG__) && defined(__KERNEL__)
 tty_print("abort() called\n");
 serial_print(SERIAL_01,"abort() called\n");
 _printtracebackex_d(1);
#endif
 ABORT((void *)EXIT_FAILURE);
}

#ifdef __LIBC_HAVE_ATEXIT

struct atexithook {
 void (*eh_hook)(int,void *);
 void  *eh_closure;
};
static int                atexit_lock = 0;
static struct atexithook *atexit_funv = NULL;
static size_t             atexit_func = 0;
#define ATEXIT_ACQUIRE while (!katomic_cmpxch(atexit_lock,0,1)) ktask_yield();
#define ATEXIT_RELEASE katomic_store(atexit_lock,0);
static void __runatexit(int exitcode) {
 struct atexithook *iter,*begin;
 ATEXIT_ACQUIRE
 iter = (begin = atexit_funv)+atexit_func;
 atexit_funv = NULL,atexit_func = 0;
 ATEXIT_RELEASE
 while (iter != begin) {
  if ((--iter)->eh_hook) {
   (*iter->eh_hook)(exitcode,iter->eh_closure);
  }
 }
 // NOTE: We purposely leak 'atexit_funv',
 //       because we don't have to care! ;)
 //free(begin);
}
__public int
on_exit(void (*func)(int   exitcode,
                     void *closure),
        void *closure) {
 struct atexithook *newvec;
 ATEXIT_ACQUIRE
 newvec = (struct atexithook *)(realloc)(atexit_funv,(atexit_func+1)*
                                         sizeof(struct atexithook));
 if (!newvec) { ATEXIT_RELEASE return -1; }
 atexit_funv = newvec;
 newvec += atexit_func++;
 newvec->eh_hook    = func;
 newvec->eh_closure = closure;
 ATEXIT_RELEASE
 return 0;
}

static void call_closure(int __unused(exitcode), void *closure) {
 (*(void(*)(void))closure)();
}
__public int atexit(void (*func)(void)) {
 return on_exit(&call_closure,(void *)func);
}
#endif /* __LIBC_HAVE_ATEXIT */

__public int system(char const *command) {
#ifdef __KERNEL__
 (void)command; // TOOD
 return 0;
#else
 typedef int (*pcmd_system)(char const *text, size_t size, uintptr_t *errorcode);
 static mod_t stored_module = MOD_ALL;
 static pcmd_system stored_cmd_system = NULL;
 uintptr_t exitcode; int error,childfd;
 pcmd_system cmd_system;
 /* Lazily load libcmd to get ahold of the system executor.
  * >> If that fails, fall back to executing 'sh -c' */
 if ((cmd_system = katomic_load(stored_cmd_system)) == NULL) {
  mod_t module2,module = katomic_load(stored_module);
  if (module == MOD_ALL) {
   module = mod_open("libcmd.so",MOD_OPEN_NONE);
   module2 = katomic_cmpxch_val(stored_module,MOD_ALL,module);
   if (module2 != MOD_ALL) { mod_close(module); module = module2; }
  }
  *(void **)&cmd_system = mod_sym(module,"cmd_system");
  katomic_cmpxch(stored_cmd_system,NULL,cmd_system);
 }

 if (cmd_system) {
  error = (*cmd_system)(command,strlen(command),&exitcode);
  /* Translate errno to bash-errors:
   * http://www.tldp.org/LDP/abs/html/exitcodes.html
   */
  if __unlikely(error == -1) switch (errno) {
   case ESYNTAX: return 2;
   case ENOEXEC:
   case ENODEP : return 126;
   case ENOENT : return 127;
   default     : return 1;
  }
  return error;
 }
 if ((childfd = task_fork()) == 0) {
  // Child task
  char *argv[] = {
   (char *)"sh",
   (char *)"-c",
   (char *)command,
   (char *)NULL,
  };
  execv("/bin/sh",argv);
  _exit(127); // STD wants it this way...
 }
 if __unlikely(childfd == -1) return -1;
 error = proc_join(childfd,&exitcode);
 if __unlikely(KE_ISERR(error)) {
  /* BAIL! */
  ktask_terminate(childfd,NULL,KTASKOPFLAG_TREE);
  kfd_close(childfd);
  return -1;
 }
 kfd_close(childfd);
 return (int)exitcode;
#endif
}

#ifndef __CONFIG_MIN_LIBC__
__local void xch(char *a, char *b, size_t s) {
 char temp;
 while (s--) {
  temp = *a;
  *a   = *b;
  *b   = temp;
  ++a,++b;
 }
}

__public void qsort(void *__restrict base, size_t num, size_t size,
                    int (*compar)(void const *p1, void const *p2)) {
 size_t i,j;
#define E(i) ((void *)((uintptr_t)base+(i)*size))
 /* I'm too lazy to do something fast right now... */
 for (i = 0; i < num; ++i) {
  for (j = i; j < num; ++j) {
   void *a = E(j),*b = E(i);
   if ((*compar)(a,b) < 0) xch((char *)a,(char *)b,size);
  }
 }
#undef E
}
#endif /* !__CONFIG_MIN_LIBC__ */


#ifndef __CONFIG_MIN_LIBC__
__public div_t div(int numer, int denom) {
 div_t result;
 assertf(denom != 0,"ERROR: Divide by ZERO");
 result.quot = numer / denom;
 result.rem  = numer % denom;
 if (numer >= 0 && result.rem < 0) {
  ++result.quot;
  result.rem -= denom;
 }
 return result;
}
__public ldiv_t ldiv(long numer, long denom) {
 ldiv_t result;
 assertf(denom != 0,"ERROR: Divide by ZERO");
 result.quot = numer / denom;
 result.rem  = numer % denom;
 if (numer >= 0 && result.rem < 0) {
  ++result.quot;
  result.rem -= denom;
 }
 return result;
}
#ifndef __NO_longlong
__public lldiv_t lldiv(long long numer, long long denom) {
 lldiv_t result;
 assertf(denom != 0,"ERROR: Divide by ZERO");
 result.quot = numer / denom;
 result.rem  = numer % denom;
 if (numer >= 0 && result.rem < 0) {
  ++result.quot;
  result.rem -= denom;
 }
 return result;
}
#endif /* !__NO_longlong */
#endif /* !__CONFIG_MIN_LIBC__ */



#ifndef __KERNEL__
// Make these public
__public long syscall(long sysno, ...);
#ifdef __KOS_HAVE_NTSYSCALL
__public long __nt_syscall(long sysno, ...);
#endif /* __KOS_HAVE_NTSYSCALL */
#ifdef __KOS_HAVE_UNIXSYSCALL
__public long __unix_syscall(long sysno, ...);
#endif /* __KOS_HAVE_UNIXSYSCALL */

// Undef malloc+realloc because of intentional leaks below...
__local char *kos_alloccmdtext(void) {
 char *result,*newresult; size_t bufsize,reqsize;
 kerrno_t error;
 bufsize = 512*sizeof(char);
 result = (char *)(malloc)(bufsize);
 if __unlikely(!result) return NULL;
again:
 error = kproc_getcmd(kproc_self(),result,bufsize,&reqsize);
 if __unlikely(KE_ISERR(error)) goto err;
 if (reqsize == bufsize) return result;
 newresult = (char *)(realloc)(result,reqsize);
 if __unlikely(!newresult) goto err_r;
 if (reqsize > bufsize) {
  bufsize = reqsize;
  result = newresult;
  goto again;
 }
 return newresult;
err:   __set_errno(-error);
err_r: free(result);
 return NULL;
}

static char *cmd_default[] = {NULL};

__public void __libc_get_argv(struct __libc_args *a) {
 char *text,*iter,**argv,**dest; size_t argc;
 assert(a);
 if __unlikely((text = kos_alloccmdtext()) == NULL) goto use_empty;
 argc = 0,iter = text;
 for (; *iter; iter = strend(iter)+1) ++argc;
 if (!argc) goto use_empty;
 /* Make space for one padding NULL entry (for convenience). */
 argv = (char **)(malloc)((argc+1)*sizeof(char *));
 if __unlikely(!argv) goto use_empty;
 iter = text,dest = argv;
 for (; *iter; iter = strend(iter)+1) *dest++ = iter;
 assert(dest == argv+argc);
 *dest = NULL;

 a->__c_argc = argc;
 a->__c_argv = argv;
 return;
use_empty:
 a->__c_argc = __compiler_ARRAYSIZE(cmd_default);
 a->__c_argv = cmd_default;
}


extern void user_initialize_environ(void);
extern void user_initialize_dlmalloc(void);

__public void __libc_init(void) {
 user_initialize_dlmalloc();
#ifdef __LIBC_HAVE_ATEXIT
#ifdef __LIBC_HAVE_DEBUG_MALLOC
 /* Setup an atexit hook to dump memory leaks when debugging. */
 atexit(&_malloc_printleaks_d);
#endif /* __LIBC_HAVE_DEBUG_MALLOC */
#endif /* __LIBC_HAVE_ATEXIT */

 user_initialize_environ();
}
#endif

__DECL_END

#endif /* !__STDLIB_C__ */
