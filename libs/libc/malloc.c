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
#ifndef __MALLOC_C__
#define __MALLOC_C__ 1
#undef __STDC_PURE__

/* Reveal the true nature of a mall-block. */
#define _mallblock_d  mallhead

#include <kos/config.h>
/* v define to quickly disable mall-blocks,
     and use dlmalloc directly. */
//#undef __LIBC_HAVE_DEBUG_MALLOC


#include <kos/compiler.h>
#include <kos/syslog.h>
#ifdef __KERNEL__
#include <kos/kernel/debug.h>
#include <kos/kernel/task.h>
#include <kos/kernel/tty.h>
#endif

#include <format-printer.h>
#include <alloca.h>
#include <malloc.h>
#include <proc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <traceback.h>

/* Get rid of any left over debug macros */
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef memalign
#undef strdup
#undef strndup
#undef memdup
#undef _memdup
#undef mallopt
#undef malloc_trim
#undef malloc_usable_size
#undef posix_memalign
#undef valloc
#undef pvalloc
#undef strdupf
#undef _strdupf
#undef vstrdupf
#undef _vstrdupf
#undef _mallblock_traceback_d
#undef _malloc_enumblocks_d
#undef _malloc_validate_d
#undef __mallblock_getattrib_d
#undef _malloc_printleaks_d

__DECL_BEGIN

/* Configure dlmalloc's exports/bindings */
#ifdef __LIBC_HAVE_DEBUG_MALLOC
#define USE_DL_PREFIX
/* These functions don't have a debug-hook
 * >> always link it directly into dlmalloc. */
#define dlmallopt      mallopt
#define dlmalloc_trim  malloc_trim
#ifndef __KERNEL__
#define dlmallinfo     mallinfo
#define dlmalloc_stats malloc_stats
#endif
#else
/* Mark all malloc-functions as public.
 * >> These will be implemented directly by dlmalloc,
 *    as we are not #defining 'USE_DL_PREFIX' */
__public __crit void *malloc(size_t s);
__public __crit void *calloc(size_t n, size_t s);
__public __crit void *realloc(void *__restrict p, size_t s);
__public __crit void *memalign(size_t alignment, size_t bytes);
__public __crit int   posix_memalign(void **__restrict memptr, size_t alignment, size_t bytes);
__public __crit void  free(void *__restrict p);
__public       size_t malloc_usable_size(void *__restrict p);
__public __crit void *pvalloc(size_t bytes);
__public __crit void *valloc(size_t bytes);
#endif

__public extern int mallopt(int,int);
__public extern int malloc_trim(size_t);
#ifndef __KERNEL__
__public extern struct mallinfo mallinfo(void);
__public extern void malloc_stats(void);
#endif

__DECL_END

/* Pull in dlmalloc and use it as system-allocator. */
#include "dlmalloc.inl"

#ifdef __LIBC_HAVE_DEBUG_MALLOC
/* Headers required for the debug mall implementation */
#include <alloca.h>
#include <math.h>
#include <stdio.h>
#include <traceback.h>
#include <stdint.h>
#include <errno.h>
#ifdef __KERNEL__
#include <kos/kernel/debug.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/sched_yield.h>
#else
#include <sched.h>
#include <kos/atomic.h>
#endif
#endif /* __LIBC_HAVE_DEBUG_MALLOC */

__DECL_BEGIN

#ifdef __LIBC_HAVE_DEBUG_MALLOC
/* ===                           === */
/* *** BEGIN MALL IMPLEMENTATION *** */
/* ===                           === */


/* === BEGIN MALL CONFIG === */

/* Enable internal assertions (Although if you
 * think mall's responsible for your code not
 * working, it'll probably be your code...) */
#define MALL_INTERNAL_DEBUG   0

/* Amount of bytes immediately before a user-block (to detect write-underflow). */
#define MALL_HEADERSIZE       16

/* Amount of bytes immediately after a user-block (to detect write-overflow). */
#define MALL_FOOTERSIZE       16

/* Validate all allocated memory every 'MALL_VALIDFREQ'th call
 * to any allocating/freeing function. (Define to 0 to disable)
 * WARNING: Validating all dynamic memory is __VERY__ expensive!
 * >> If at all, this ~really~ needs to be a stupendously high value */
#define MALL_VALIDFREQ        1024

/* Write a syslog entry whenever a periodic memory validation is performed. */
#define MALL_VALIDFREQ_SYSLOG  defined(__KERNEL__)

/* === END MALL CONFIG === */


/* === BEGIN MALL RANDOM === */
/* The following numbers were generated using the best algorithm there is:
 * The human brain. - Yes, there should be absolutely no pattern to these! */
#if 0
#define MALL_HEADERBYTE(i)   0x65
#define MALL_FOOTERBYTE(i)   0xB6
#else
static byte_t const mall_header_seed[4] = {0x65,0xB6,0xBD,0x5A};
static byte_t const mall_footer_seed[4] = {0xCF,0x6A,0xB7,0x97};
#define MALL_HEADERBYTE(i)   (mall_header_seed[(i) % 4]^((0xff >> (i) % 8)*(i))) /* Returns the i-th control byte for mall-headers. */
#define MALL_FOOTERBYTE(i)   (mall_footer_seed[(i) % 4]^((0xff >> (i) % 7)*((i)+1))) /* Returns the i-th control byte for mall-footers. */
#endif
/* === END MALL RANDOM === */


/* === BEGIN MALL SYSTEM HOOKS === */
#define MALL_SYSMALLOC        dlmalloc
#define MALL_SYSYIELD         task_yield()
#define MALL_SYSFREE          dlfree
#define MALL_SYSUSABLESIZE    dlmalloc_usable_size
#define MALL_SYSMUSTLOCK      0
#define MALL_SYSPAGESIZE     (ensure_initialization(),mparams.page_size)
#define MALL_SYSMALLOC_ALIGN  MALLOC_ALIGNMENT
#ifdef __KERNEL__
#define MALL_SYSPRINTF(...)  (serial_printf(SERIAL_01,__VA_ARGS__),tty_printf(__VA_ARGS__))
#elif 1
#define MALL_SYSPRINTF(...)  (dprintf(STDERR_FILENO,__VA_ARGS__),k_syslogf(KLOG_ERROR,__VA_ARGS__))
#else
#define MALL_SYSPRINTF(...)   dprintf(STDERR_FILENO,__VA_ARGS__)
#endif
/* === END MALL SYSTEM HOOKS === */


#ifndef KDEBUG_SOURCEPATH_PREFIX
#define KDEBUG_SOURCEPATH_PREFIX "../"
#endif

#if MALL_INTERNAL_DEBUG
#define MALL_ASSERT   assert
#define MALL_ASSERTF  assertf
#elif defined(__OPTIMIZE__)
#define MALL_ASSERT(expr)      __compiler_assume(expr)
#define MALL_ASSERTF(expr,...) __compiler_assume(expr)
#else
#define MALL_ASSERT(expr)      (void)0
#define MALL_ASSERTF(expr,...) (void)0
#endif


/* ... And I thought I knew what I was doing when I created debug_new.
 *     Yet then it can be so easy to create something so much better ... */

static int capturetb_sizewalker(void const *__restrict instruction_pointer,
                                void const *__restrict frame_address,
                                size_t frame_index, size_t *closure) {
 if (frame_index >= *closure) *closure = frame_index+1;
 return 0;
}
static int capturetb_walker(void const *__restrict instruction_pointer,
                            void const *__restrict frame_address,
                            size_t frame_index, void const **closure) {
 closure[frame_index] = instruction_pointer;
 return 0;
}
static __noinline size_t capturetbsize(size_t skip) {
 size_t result = 0;
 _walktraceback_d((_ptraceback_stackwalker_d)&capturetb_sizewalker,
                  NULL,&result,skip+1);
 return result;
}
static __noinline void capturetb(void const **__restrict addrvec, size_t skip) {
 _walktraceback_d((_ptraceback_stackwalker_d)&capturetb_walker,
                  NULL,addrvec,skip+1);
}

/* NOTE: mall blocks with a NULL file are not tracked.
 *    >> Using this (quite simple) rule, the user can intentionally
 *       leak memory by simply #undef-ing malloc/realloc/etc., while
 *       having the returned memory behave the same as that returned
 *       by a tracked allocator (e.g.: '_malloc_d').
 *    >> The returned memory can still be freed as usual, with it
 *       not mattering if '_free_d' or 'free' is used:
 * >> #undef malloc
 * >> void *intentional_leak = malloc(42); // This leak will not be tracked
 */

#define MALL_MAGIC 0xE77A61C3 /* <MAGIC> */

struct mallhead {
 __u32            mh_magic;  /*< == MALL_MAGIC. */
 __u32            mh_refcnt; /*< Reference counter (required for handling free() while enumerating). */
 void            *mh_base;   /*< [1..1] Actually allocated pointer. */
 struct mallhead *mh_prev;   /*< [0..1][lock(mall_lock)] Previously allocated header. */
 struct mallhead *mh_next;   /*< [0..1][lock(mall_lock)] Next allocated header. */
 char const      *mh_file;   /*< [0..1][const] Allocation file (If NULL, this entry isn't tracked). */
 int              mh_line;   /*< [const] Allocation line. */
 char const      *mh_func;   /*< [0..1][const] Allocation function. */
 size_t           mh_usize;  /*< [const] Size of the userdata block. */
 size_t           mh_tsz;    /*< [const] Size of the malloc traceback. */
 void const      *mh_tb[1];  /*< [0..mh_tsz][const] Malloc traceback instruction pointers (inline) */
};
struct malltail {
 struct mallhead *mt_head;  /*< [1..1] Pointer to the malloc header. */
 /* MALL_HEADERSIZE is located here. */
 /* User-data is located here. */
 /* MALL_FOOTERSIZE is located here. */
};
#define mall_headtail_size \
 (offsetof(struct mallhead,mh_tb)+sizeof(struct malltail))

/* Convert between various part of mall-blocks. */
#define mall_head2tail(self) \
 ((struct malltail *)((uintptr_t)(self)+\
   offsetof(struct mallhead,mh_tb)+\
  (self)->mh_tsz*sizeof(void *)))
#define mall_tail2fill(self) (byte_t *)(((struct malltail *)(self))+1)
#define mall_fill2tail(self) (((struct malltail *)(self))-1)
#if MALL_HEADERSIZE
#define mall_fill2user(self) (void *)((uintptr_t)self+MALL_HEADERSIZE)
#define mall_user2fill(self) ((byte_t *)((uintptr_t)(self)-MALL_HEADERSIZE))
#else
#define mall_fill2user(self) ((void *)(self))
#define mall_user2fill(self) ((byte_t *)(self))
#endif
#define mall_tail2user(self) mall_fill2user(mall_tail2fill(self))
#define mall_head2user(self) mall_tail2user(mall_head2tail(self))
#define mall_user2tail(self) (((struct malltail *)mall_user2fill(self))-1)
#define mall_head2foot(self) ((byte_t *)((uintptr_t)mall_head2user(self)+(self)->mh_usize))

#define mallhead_size(self) (offsetof(struct mallhead,mh_tb)+(self)->mh_tsz*sizeof(void *))

/* Amount of skipped entires in tracebacks
 * (Adjustment to not include frame entries from mall itself). */
#define MALL_TBOFF           2

#if MALL_SYSMUSTLOCK
static __atomic int mall_syslock = 0;
#ifdef KTASK_SPIN
#   define MALL_SYSACQUIRE KTASK_SPIN(katomic_cmpxch(mall_syslock,0,1));
#else
#   define MALL_SYSACQUIRE { while (!katomic_cmpxch(mall_syslock,0,1)) MALL_SYSYIELD; }
#endif
#   define MALL_SYSRELEASE { katomic_store(mall_syslock,0); }
#else
#   define MALL_SYSACQUIRE /* nothing */
#   define MALL_SYSRELEASE /* nothing */
#endif

struct mallhead;

#if MALL_VALIDFREQ > 1
static __atomic int mall_currfreq = MALL_VALIDFREQ;
static void mall_validate(void) {
#if MALL_VALIDFREQ_SYSLOG
 k_syslogf(KLOG_INFO,"[MALL] Performing periodic memory validation\n");
#endif
 _malloc_validate_d();
}
#define MALL_FREQ() \
do{ int old_freq,new_freq;\
    do old_freq = mall_currfreq,\
       new_freq = old_freq > 0 ? old_freq-1 : MALL_VALIDFREQ;\
    while (!katomic_cmpxch(mall_currfreq,old_freq,new_freq));\
    if (old_freq <= 0) mall_validate();\
}while(0)
#elif MALL_VALIDFREQ == 1
#   define MALL_FREQ() _malloc_validate_d()
#else /* ... */
#   define MALL_FREQ() (void)0
#endif /* !MALL_VALIDFREQ */

static struct mallhead *mall_top  = NULL; /*< [0..1] Last allocated pointer. */
static __atomic int     mall_lock = 0;    /*< Spinlock used to lock mall_top. */
#ifdef KTASK_SPIN
#   define MALL_LOCK_ACQUIRE { KTASK_SPIN(katomic_cmpxch(mall_lock,0,1)); }
#else
#   define MALL_LOCK_ACQUIRE { while (!katomic_cmpxch(mall_lock,0,1)) MALL_SYSYIELD; }
#endif
#   define MALL_LOCK_RELEASE { katomic_store(mall_lock,0); }


__local __crit void mall_insert(struct mallhead *__restrict head) {
 head->mh_next = NULL;
 if __unlikely(!head->mh_file) {
  /* This mall-entry isn't supposed to be tracked. */
  head->mh_prev = NULL;
  return;
 }
 MALL_LOCK_ACQUIRE
 head->mh_prev = mall_top;
 if (mall_top) mall_top->mh_next = head;
 mall_top = head;
 MALL_LOCK_RELEASE
}
__local __crit void mall_remove_unlocked(struct mallhead *__restrict head) {
 if __unlikely(!head->mh_file) return;
 MALL_ASSERTF((head->mh_next == NULL) == (head == mall_top)
             ,"head->mh_file = %s\n"
              "head->mh_line = %d\n"
              "head->mh_func = %s\n"
              "head->mh_next = %p\n"
              "head          = %p\n"
              "mall_top      = %p\n"
             ,head->mh_file,head->mh_line,head->mh_func
             ,head->mh_next,head,mall_top);
 if (head->mh_next) head->mh_next->mh_prev = head->mh_prev;
 else mall_top = head->mh_prev;
 if (head->mh_prev) head->mh_prev->mh_next = head->mh_next;
 head->mh_prev = NULL;
 head->mh_next = NULL;
}

static int _mallblock_do_traceback_d(struct mallhead const *__restrict self,
                                     _ptraceback_stackwalker_d callback,
                                     void *closure) {
 void const *const *iter,*end;
 size_t pos = 0; int error = 0;
 end = (iter = self->mh_tb)+self->mh_tsz;
 while (iter != end) {
  error = (*callback)(*iter,NULL,pos,closure);
  if __unlikely(error != 0) break;
  ++iter,++pos;
 }
 return error;
}

static int
printleaks_tb_callback(void const *__restrict addr,
                       void const *__restrict __unused(frame_address),
                       size_t index, void *__unused(closure)) {
 MALL_SYSPRINTF("#!$ addr2line(%p) '{file}({line}) : {func} : [%Ix] : %p'\n",
            ((uintptr_t)addr)-1,index,addr);
 return 0;
}
static int mall_printleak(struct mallhead const *__restrict head, char const *reason) {
 MALL_SYSPRINTF("##################################################\n"
             KDEBUG_SOURCEPATH_PREFIX "%s(%d) : %s : %s %Iu bytes at %p\n"
            ,head->mh_file,head->mh_line
            ,head->mh_func,reason,head->mh_usize
            ,mall_head2user(head));
#ifdef __KERNEL__
 {
  int result = _mallblock_do_traceback_d(head,&printleaks_tb_callback,NULL);
  debug_hexdump(mall_head2user(head),head->mh_usize);
  return result;
 }
#else
 return _mallblock_do_traceback_d(head,&printleaks_tb_callback,NULL);
#endif
}



#define VALIDATE(expr,...) __assert_atf("...",skip+1,expr,__VA_ARGS__)
#define VALIDATE_MALLHEAD_TAIL_DIFF(head,tail) \
 VALIDATE((uintptr_t)(head) < (uintptr_t)(tail)\
          ,"Expected mallhead address %p below malltail %p (but lies %Iu bytes above)"\
          ,head,tail,(uintptr_t)(head)-(uintptr_t)(tail))

#define VALIDATE_MALLHEAD_TAIL(head,tail) \
do{ kerrno_t errid = kmem_validate(head,(uintptr_t)(tail)-(uintptr_t)(head));\
    VALIDATE(errid == KE_OK\
            ,"Invalid mallhead...malltail memory range: %p+%Iu ... %p (%s)"\
            ,head,(uintptr_t)(tail)-(uintptr_t)(head)\
            ,tail,kassertmem_msg(errid));\
}while(0)
#define VALIDATE_MALLHEAD_MAGIC(head,user) \
 VALIDATE((head)->mh_magic == MALL_MAGIC\
         ,"Invalid mallhead magic at %p (head: %p; user: %p): Expected %I32x, but got %I32x"\
         ,&(head)->mh_magic,head,user,MALL_MAGIC,(head)->mh_magic)
#define VALIDATE_MALLHEAD_REFCNT(head,user) \
 VALIDATE((head)->mh_refcnt != 0\
         ,"Invalid mallhead refcnt (0) at %p (head: %p; user: %p) (Was free() called twice?)"\
         ,&(head)->mh_refcnt,head,user)
#define VALIDATE_MALLHEAD_BASE(head,user) \
 VALIDATE((uintptr_t)(head)->mh_base <= (uintptr_t)(head)\
          ,"Expected mallbase address %p below mallhead %p (user: %p) (but lies %Iu bytes above)"\
          ,(head)->mh_base,head,user,(uintptr_t)(head)->mh_base-(uintptr_t)(head));
#define VALIDATE_MALLHEAD_USER(head,user) \
do{ kerrno_t errid = kmem_validate(user,(head)->mh_usize);\
    VALIDATE(errid == KE_OK\
            ,"Invalid allocated user-memory range (head: %p): %p+%Iu ... %p (%s)"\
            ,head,user,(head)->mh_usize,(uintptr_t)(user)+(head)->mh_usize\
            ,kassertmem_msg(errid));\
}while(0)

#if MALL_HEADERSIZE
#define VALIDATE_MALLHEAD_HEADER(head,user,tail) \
do{ byte_t const *iter,*end,*start; byte_t expected_byte;\
    end = (iter = start = mall_tail2fill(tail))+MALL_HEADERSIZE;\
    for (; iter != end; ++iter) {\
     expected_byte = MALL_HEADERBYTE(iter-start);\
     if __unlikely(*iter != expected_byte) {\
      MALL_SYSPRINTF("\n[HEAP VIOLATION] See reference to allocated block:\n");\
      mall_printleak(head,"Allocated");\
      __assert_atf("byte != MALL_HEADERBYTE(...)",skip+1,0\
                  ,"[MALL:HEAD] Invalid byte in mallheader-filler (head: %p; user: %p) "\
                              "(byte %Iu/%Iu is %#I8x (%I8u) instead of %#I8x (%I8u))"\
                  ,head,user,(size_t)(iter-start),(size_t)(MALL_HEADERSIZE-1)\
                  ,*iter,*iter,expected_byte,expected_byte);\
     }\
    }\
}while(0)
#else
#define VALIDATE_MALLHEAD_HEADER(head,user,tail) (void)0
#endif

#if MALL_FOOTERSIZE
#define VALIDATE_MALLHEAD_FOOTER(head,user,tail)\
do{ byte_t const *iter,*end,*start; byte_t expected_byte;\
    end = (iter = start = mall_head2foot(head))+MALL_FOOTERSIZE;\
    for (; iter != end; ++iter) {\
     expected_byte = MALL_FOOTERBYTE(iter-start);\
     if __unlikely(*iter != expected_byte) {\
      MALL_SYSPRINTF("\n[HEAP VIOLATION] See reference to allocated block:\n");\
      mall_printleak(head,"Allocated");\
      __assert_atf("byte != MALL_FOOTERBYTE(...)",skip+1,0\
                  ,"[MALL:FOOT] Invalid byte in mallfooter-filler (head: %p; user: %p) "\
                              "(byte %Iu/%Iu is %#I8x (%I8u) instead of %#I8x (%I8u))"\
                  ,head,user,(size_t)(iter-start),(size_t)(MALL_FOOTERSIZE-1)\
                  ,*iter,*iter,expected_byte,expected_byte);\
     }\
    }\
}while(0)
#else
#define VALIDATE_MALLHEAD_FOOTER(head,user,tail) (void)0
#endif

static __noinline void
mall_validatehead(int original, struct mallhead const *head, size_t skip __LIBC_DEBUG__PARAMS) {
 kerrno_t errid; size_t tb_space;
 struct malltail *tail; void *user;
 /*printf("base: %p %p\n",head->mh_base,head);*/
 errid = kmem_validate(head,offsetof(struct mallhead,mh_tb));
 VALIDATE(errid == KE_OK
         ,"Invalid mallhead...traceback memory range: %p+%Iu ... %p (%s)"
         ,head,offsetof(struct mallhead,mh_tb)
         ,(uintptr_t)head+offsetof(struct mallhead,mh_tb)
         ,kassertmem_msg(errid));
 tail = mall_head2tail(head);
 VALIDATE_MALLHEAD_TAIL(head,tail);
 user = mall_tail2user(tail);
 VALIDATE_MALLHEAD_MAGIC(head,user);
 VALIDATE_MALLHEAD_REFCNT(head,user);
 if (original) VALIDATE_MALLHEAD_BASE(head,user);
 VALIDATE_MALLHEAD_USER(head,user);
 errid = kmem_validate(head->mh_base,(uintptr_t)head-(uintptr_t)head->mh_base);
 VALIDATE(errid == KE_OK,
          "Invalid mallbase...mallhead memory range: %p+%Iu ... %p (%s)",
          head->mh_base,(uintptr_t)head-(uintptr_t)head->mh_base,
          head,kassertmem_msg(errid));
 tb_space = (uintptr_t)tail-(uintptr_t)head->mh_tb;
 VALIDATE(tb_space == head->mh_tsz*sizeof(void *),
          "Invalid stored traceback size (Stored size: %Iu; Available space: %Iu)",
          head->mh_tsz*sizeof(void *),tb_space);
 VALIDATE_MALLHEAD_HEADER(head,user,tail);
 VALIDATE_MALLHEAD_FOOTER(head,user,tail);
}

static __noinline __retnonnull struct mallhead *
mall_user2head(void *__restrict p, size_t skip __LIBC_DEBUG__PARAMS) {
 struct mallhead *result; kerrno_t errid; size_t tb_space;
 struct malltail *tail = mall_user2tail(p);
 errid = kmem_validate(tail,sizeof(struct malltail)+MALL_HEADERSIZE);
 VALIDATE(errid == KE_OK
         ,"Invalid malltail...malluser memory range: %p+%Iu ... %p (%s)"
         ,tail,sizeof(struct malltail)
         ,(void *)((__uintptr_t)tail+sizeof(struct malltail))
         ,kassertmem_msg(errid));
 result = tail->mt_head;
 VALIDATE_MALLHEAD_TAIL_DIFF(result,tail);
 VALIDATE_MALLHEAD_TAIL(result,tail);
 VALIDATE_MALLHEAD_MAGIC(result,p);
 VALIDATE_MALLHEAD_REFCNT(result,p);
 VALIDATE_MALLHEAD_BASE(result,p);
 VALIDATE_MALLHEAD_USER(result,p);
 errid = kmem_validate(result->mh_base,(uintptr_t)result-(uintptr_t)result->mh_base);
 VALIDATE(errid == KE_OK
         ,"Invalid mallbase...mallhead memory range: %p+%Iu ... %p (%s)"
         ,result->mh_base,(uintptr_t)result-(uintptr_t)result->mh_base
         ,result,kassertmem_msg(errid));
 tb_space = (uintptr_t)tail-(uintptr_t)result->mh_tb;
 VALIDATE(tb_space == result->mh_tsz*sizeof(void *)
         ,"Invalid stored traceback size (Stored size: %Iu; Available space: %Iu)"
         ,result->mh_tsz*sizeof(void *),tb_space);
 VALIDATE_MALLHEAD_HEADER(result,p,tail);
 VALIDATE_MALLHEAD_FOOTER(result,p,tail);
 return result;
}
#undef VALIDATE

/* The main mall-allocator function. */
static __crit __noinline void *mall_malloc(size_t s, size_t alignment __LIBC_DEBUG__PARAMS) {
 size_t traceback_size,headtailsize; void *result;
 struct mallhead *head; struct malltail *tail; void *baseptr;
#ifdef KTASK_CRIT_MARK
 KTASK_CRIT_MARK
#endif
 MALL_FREQ();
#if MALL_FOOTERSIZE
 /* Prevent 'malloc_usable_size()' from returning ZERO and
  * potentially causing undefined behavior later down the line... */
 if __unlikely(!s) s = 1;
#endif
 traceback_size = capturetbsize(MALL_TBOFF);
 headtailsize = mall_headtail_size+traceback_size*sizeof(void *);
 /* Overallocate to correctly handle any kind of native
  * alignment (including 1-byte; aka. not-aligned-at-all) */
 MALL_SYSACQUIRE
 baseptr = MALL_SYSMALLOC(alignment+headtailsize+MALL_HEADERSIZE+MALL_FOOTERSIZE+s);
 MALL_SYSRELEASE
 if __unlikely(!baseptr) return NULL;
 /* Align what will essentially become the malluser by the given alignment. */
 head = (struct mallhead *)(_align((uintptr_t)baseptr+headtailsize,alignment)-headtailsize);
 head->mh_magic  = MALL_MAGIC;
 head->mh_refcnt = 1;
 head->mh_base   = baseptr;
 head->mh_file   = __LIBC_DEBUG_FILE;
 head->mh_line   = __LIBC_DEBUG_LINE;
 head->mh_func   = __LIBC_DEBUG_FUNC;
 head->mh_usize  = s;
 head->mh_tsz    = traceback_size;
 capturetb(head->mh_tb,MALL_TBOFF);
 tail = mall_head2tail(head);
 tail->mt_head = head;
 mall_insert(head);
#if MALL_HEADERSIZE
 {
  byte_t *header,*iter,*end;
  header = mall_tail2fill(tail);
  end = (iter = header)+MALL_HEADERSIZE;
  for (; iter != end; ++iter) {
   *iter = MALL_HEADERBYTE(iter-header);
  }
  result = mall_fill2user(header);
 }
#else
 result = mall_tail2user(tail);
#endif
#if MALL_FOOTERSIZE
 {
  byte_t *footer,*iter,*end;
  footer = mall_head2foot(head);
  end = (iter = footer)+MALL_FOOTERSIZE;
  for (; iter != end; ++iter) {
   *iter = MALL_FOOTERBYTE(iter-footer);
  }
 }
#endif

 MALL_ASSERT((uintptr_t)head->mh_base <= (uintptr_t)head);
 MALL_ASSERT((uintptr_t)head < (uintptr_t)tail);
 MALL_ASSERT((uintptr_t)tail < (uintptr_t)result);
 /*mall_validatehead(1,head,1 __LIBC_DEBUG__FWD);*/
 return result;
}
static __crit __noinline void mall_free(void *__restrict p __LIBC_DEBUG__PARAMS) {
 struct mallhead *head;
 /* Ignore NULL-pointers. */
 if __unlikely(!p) return;
 {
#ifdef KTASK_CRIT_MARK
  KTASK_CRIT_MARK
#endif
  MALL_FREQ();
  head = mall_user2head(p,MALL_TBOFF __LIBC_DEBUG__FWD);
  MALL_LOCK_ACQUIRE
  if (!--head->mh_refcnt) {
   mall_remove_unlocked(head);
   MALL_SYSACQUIRE
   MALL_SYSFREE(head->mh_base);
   MALL_SYSRELEASE
  }
  MALL_LOCK_RELEASE
 }
}
static __noinline size_t mall_sizeof(void *__restrict p __LIBC_DEBUG__PARAMS) {
 struct mallhead *head;
 if __unlikely(!p) return 0;
 head = mall_user2head(p,MALL_TBOFF __LIBC_DEBUG__FWD);
#if MALL_FOOTERSIZE
 /* With footers, there is no relaxed usable-size area. */
 return head->mh_usize;
#elif MALL_SYSMUSTLOCK
 {
  size_t result;
  MALL_SYSACQUIRE
  result = MALL_SYSUSABLESIZE(head->mh_base)-mallhead_size(head);
  MALL_SYSRELEASE
  return result;
 }
#else
 return MALL_SYSUSABLESIZE(head->mh_base)-mallhead_size(head);
#endif
}



__public __crit void *_malloc_d(size_t s __LIBC_DEBUG__PARAMS) {
 return mall_malloc(s,MALLOC_ALIGNMENT __LIBC_DEBUG__FWD);
}
__public __crit void *_calloc_d(size_t n, size_t s __LIBC_DEBUG__PARAMS) {
 void *result = mall_malloc(n*s,MALLOC_ALIGNMENT __LIBC_DEBUG__FWD);
 if __likely(result) memset(result,0,n*s);
 return result;
}
__public __crit void *_realloc_d(void *__restrict p, size_t s __LIBC_DEBUG__PARAMS) {
 void *result;
 result = mall_malloc(s,MALLOC_ALIGNMENT __LIBC_DEBUG__FWD);
 if (p && result) {
  memcpy(result,p,min(s,mall_sizeof(p __LIBC_DEBUG__FWD)));
  mall_free(p __LIBC_DEBUG__FWD);
 }
 return result;
}
__public __crit void *_memalign_d(size_t alignment, size_t bytes __LIBC_DEBUG__PARAMS) {
 return mall_malloc(bytes,alignment __LIBC_DEBUG__FWD);
}
__public __crit int _posix_memalign_d(void **__restrict memptr, size_t alignment,
                                      size_t bytes __LIBC_DEBUG__PARAMS) {
 size_t d = alignment / sizeof(void*);
 size_t r = alignment % sizeof(void*);
 if (r != 0 || d == 0 || (d & (d-SIZE_T_ONE)) != 0) return EINVAL;
 *memptr = mall_malloc(bytes,alignment __LIBC_DEBUG__FWD);
 return *memptr ? EOK : ENOMEM;
}
__public __crit void _free_d(void *p __LIBC_DEBUG__PARAMS) { mall_free(p __LIBC_DEBUG__FWD); }
__public __crit char *_strdup_d(char const *__restrict s __LIBC_DEBUG__PARAMS) {
 size_t slen = strlen(s)+1;
 char *result = (char *)mall_malloc(slen*sizeof(char),MALLOC_ALIGNMENT __LIBC_DEBUG__FWD);
 if (result) memcpy(result,s,slen*sizeof(char));
 return result;
}
__public __crit char *_strndup_d(char const *__restrict s, size_t maxchars __LIBC_DEBUG__PARAMS) {
 size_t slen = strnlen(s,maxchars);
 char *result = (char *)mall_malloc((slen+1)*sizeof(char),MALLOC_ALIGNMENT __LIBC_DEBUG__FWD);
 if (result) { memcpy(result,s,slen*sizeof(char)); result[slen] = '\0'; }
 return result;
}
__public __crit void *_memdup_d(void const *__restrict p, size_t bytes __LIBC_DEBUG__PARAMS) {
 char *result = (char *)mall_malloc(bytes,MALLOC_ALIGNMENT __LIBC_DEBUG__FWD);
 if (result) memcpy(result,p,bytes);
 return result;
}
__public __crit size_t _malloc_usable_size_d(void *__restrict p __LIBC_DEBUG__PARAMS) {
 return mall_sizeof(p __LIBC_DEBUG__FWD);
}
__public __crit void *_pvalloc_d(size_t bytes __LIBC_DEBUG__PARAMS) {
 size_t pagesz = MALL_SYSPAGESIZE;
 return mall_malloc((bytes + pagesz - SIZE_T_ONE) & ~(pagesz - SIZE_T_ONE),pagesz __LIBC_DEBUG__FWD);
}
__public __crit void *_valloc_d(size_t bytes __LIBC_DEBUG__PARAMS) {
 size_t pagesz = MALL_SYSPAGESIZE;
 return mall_malloc(bytes,pagesz __LIBC_DEBUG__FWD);
}


//////////////////////////////////////////////////////////////////////////
// Non-debug versions still use debug allocator (for binary compability)
__public __crit void *malloc(size_t s) {
 return mall_malloc(s,MALLOC_ALIGNMENT __LIBC_DEBUG__NULL);
}
__public __crit void *calloc(size_t n, size_t s) {
 void *result = mall_malloc(n*s,MALLOC_ALIGNMENT __LIBC_DEBUG__NULL); ;
 if __likely(result) memset(result,0,n*s);
 return result;
}
__public __crit void *realloc(void *__restrict p, size_t s) {
 void *result;
 result = mall_malloc(s,MALLOC_ALIGNMENT __LIBC_DEBUG__NULL);
 if (p && result) {
  memcpy(result,p,min(s,mall_sizeof(p __LIBC_DEBUG__NULL)));
  mall_free(p __LIBC_DEBUG__NULL);
 }
 return result;
}
__public __crit void *memalign(size_t alignment, size_t bytes) {
 return mall_malloc(bytes,alignment __LIBC_DEBUG__NULL);
}
__public __crit int posix_memalign(void **__restrict memptr,
                                   size_t alignment, size_t bytes) {
 size_t d = alignment / sizeof(void*);
 size_t r = alignment % sizeof(void*);
 if (r != 0 || d == 0 || (d & (d-SIZE_T_ONE)) != 0) return EINVAL;
 *memptr = mall_malloc(bytes,alignment __LIBC_DEBUG__NULL);
 return *memptr ? EOK : ENOMEM;
}
__public __crit void free(void *__restrict p) {
 mall_free(p __LIBC_DEBUG__NULL);
}
__public __crit char *strdup(char const *__restrict s) {
 size_t slen = strlen(s)+1;
 char *result = (char *)mall_malloc(slen*sizeof(char),MALLOC_ALIGNMENT __LIBC_DEBUG__NULL);
 if (result) memcpy(result,s,slen*sizeof(char));
 return result;
}
__public __crit char *strndup(char const *__restrict s, size_t maxchars) {
 size_t slen = strnlen(s,maxchars);
 char *result = (char *)mall_malloc((slen+1)*sizeof(char),MALLOC_ALIGNMENT __LIBC_DEBUG__NULL);
 if (result) { memcpy(result,s,slen*sizeof(char)); result[slen] = '\0'; }
 return result;
}
__public __crit void *_memdup(void const *__restrict p, size_t bytes) {
 char *result = (char *)mall_malloc(bytes,MALLOC_ALIGNMENT __LIBC_DEBUG__NULL);
 if (result) memcpy(result,p,bytes);
 return result;
}
__public size_t malloc_usable_size(void *__restrict p) {
 return mall_sizeof(p __LIBC_DEBUG__NULL);
}
__public __crit void *pvalloc(size_t bytes) {
 size_t pagesz = MALL_SYSPAGESIZE;
 return mall_malloc((bytes + pagesz - SIZE_T_ONE) & ~(pagesz - SIZE_T_ONE),pagesz __LIBC_DEBUG__NULL);
}
__public __crit void *valloc(size_t bytes) {
 size_t pagesz = MALL_SYSPAGESIZE;
 return mall_malloc(bytes,pagesz __LIBC_DEBUG__NULL);
}

#define DO_STRDUPF(result,format,args) \
do{\
 /* Minimal implementation (Not meant for speed) */\
 va_list args_copy; size_t result_size;\
 va_copy(args_copy,args);\
 result_size = (vsnprintf(NULL,0,format,args_copy)+1)*sizeof(char);\
 va_end(args_copy);\
 result = (char *)mall_malloc(result_size,MALLOC_ALIGNMENT DEBUG_ARGS);\
 if (result) vsnprintf(result,result_size,format,args);\
}while(0)

#define DEBUG_ARGS  __LIBC_DEBUG__FWD
__public __crit char *_strdupf_d(__LIBC_DEBUG_PARAMS_ char const *__restrict format, ...) {
 va_list args; char *result;
 va_start(args,format);
 DO_STRDUPF(result,format,args);
 va_end(args);
 return result;
}
__public __crit char *_vstrdupf_d(char const *__restrict format, va_list args __LIBC_DEBUG__PARAMS) {
 char *result;
 DO_STRDUPF(result,format,args);
 return result;
}
#undef DEBUG_ARGS
#define DEBUG_ARGS  __LIBC_DEBUG__NULL
__public __crit char *_strdupf(char const *__restrict format, ...) {
 va_list args; char *result;
 va_start(args,format);
 DO_STRDUPF(result,format,args);
 va_end(args);
 return result;
}
__public __crit char *_vstrdupf(char const *__restrict format, va_list args) {
 char *result;
 DO_STRDUPF(result,format,args);
 return result;
}
#undef DEBUG_ARGS
#undef DO_STRDUPF


//////////////////////////////////////////////////////////////////////////
// Public mallblock interface
static int printleaks_callback(struct mallhead *__restrict block, void *closure);
static int validate_callback(struct mallhead *__restrict block, void *closure);
int _malloc_enumblocks_ex_d(void *checkpoint,
                            int (*callback)(struct mallhead *__restrict block,
                                            void *closure),
                            void *closure, size_t tb_skip) {
#define PASS_COPY 0
 struct mallhead *blocks,*iter,*next,*oldtop;
#if PASS_COPY
 struct mallhead *mallbuf;
 size_t max_stacksize = 0;
#endif
 int error = 0;
 if __unlikely(!callback) return 0;
#ifdef __KERNEL__
 NOIRQ_BEGIN
#endif
 MALL_LOCK_ACQUIRE
 oldtop = mall_top,mall_top = NULL;
 assert(!oldtop || !oldtop->mh_next);
 if (checkpoint) {
  struct mallhead *checkpoint_head;
  checkpoint_head = mall_user2head(checkpoint,tb_skip+1 __LIBC_DEBUG__NULL);
  blocks = checkpoint_head->mh_prev;
 } else {
  /* Enumerate all blocks. */
  blocks = oldtop;
 }
#if PASS_COPY
 for (iter = blocks; iter; iter = iter->mh_prev) {
  if (iter->mh_tsz > max_stacksize) max_stacksize = iter->mh_tsz;
 }
 mallbuf = (struct mallhead *)alloca(mall_headtail_size+
                                     max_stacksize*sizeof(void *));
#endif
 iter = blocks;
 while (iter) {
  MALL_ASSERT(iter->mh_tsz <= max_stacksize);
#if PASS_COPY
  /* For safety, we keep a copy of the buffer on the stack. */
  memcpy(mallbuf,iter,mall_headtail_size+
         iter->mh_tsz*sizeof(void *));
#endif
  ++iter->mh_refcnt;
  MALL_LOCK_RELEASE
  /* Run the user-provided callback on the given block. */
#if PASS_COPY
  error = (*callback)(mallbuf,closure);
#else
  error = (*callback)(iter,closure);
#endif
  MALL_LOCK_ACQUIRE
  next = iter->mh_prev;
  /* Perform a full validation of 'iter'. */
  mall_user2head(mall_head2user(iter),tb_skip+1 __LIBC_DEBUG__NULL);
  if (!--iter->mh_refcnt) {
   mall_remove_unlocked(iter);
   MALL_SYSACQUIRE
   MALL_SYSFREE(iter->mh_base);
   MALL_SYSRELEASE
  }
  /* Stop iterating if the user's callback returned non-zero. */
  if __unlikely(error != 0) break;
  iter = next;
 }
 if (oldtop) {
  /* Restore the popped list of blocks. */
  if (mall_top) {
   /* Prepend the popped list of blocks before the new list. */
   iter = mall_top;
   while (iter->mh_prev) iter = iter->mh_prev;
   iter->mh_prev = oldtop;
   oldtop->mh_next = iter;
  } else {
   mall_top = oldtop;
  }
 }
 MALL_LOCK_RELEASE
#ifdef __KERNEL__
 NOIRQ_END
#endif
 return error;
}
__public int _malloc_enumblocks_d(void *checkpoint,
                                  int (*callback)(struct mallhead *__restrict block,
                                                  void *closure),
                                  void *closure) {
 return _malloc_enumblocks_ex_d(checkpoint,callback,closure,1);
}

__public void *__mallblock_getattrib_d(struct mallhead const *__restrict self, int attrib) {
 kassertobj(self);
 mall_validatehead(0,self,1 __LIBC_DEBUG__NULL);
 switch (attrib) {
  case __MALLBLOCK_ATTRIB_FILE: return (void *)self->mh_file;
  case __MALLBLOCK_ATTRIB_LINE: return (void *)(uintptr_t)self->mh_line;
  case __MALLBLOCK_ATTRIB_FUNC: return (void *)self->mh_func;
  case __MALLBLOCK_ATTRIB_SIZE: return (void *)(uintptr_t)self->mh_usize;
  case __MALLBLOCK_ATTRIB_ADDR: return mall_head2user(self);
  default: break;
 }
 return NULL;
}
__public int _mallblock_traceback_d(struct mallhead *__restrict self,
                                    _ptraceback_stackwalker_d callback,
                                    void *closure) {
 mall_validatehead(0,self,1 __LIBC_DEBUG__NULL);
 return _mallblock_do_traceback_d(self,callback,closure);
}

static int printleaks_callback(struct mallhead *__restrict block, void *closure) {
 mall_validatehead(0,block,3 __LIBC_DEBUG__NULL);
 return mall_printleak(block,"Leaked");
}
static int validate_callback(struct mallhead *__restrict block, void *closure) {
 mall_validatehead(0,block,3 __LIBC_DEBUG__NULL);
 return 0;
}

__public void _malloc_printleaks_d(void) { _malloc_enumblocks_ex_d(NULL,&printleaks_callback,NULL,1); }
__public void _malloc_validate_d(void) { _malloc_enumblocks_ex_d(NULL,&validate_callback,NULL,1); }

/* ===                         === */
/* *** END MALL IMPLEMENTATION *** */
/* ===                         === */
#else

/* Map debug malloc functions as dumb aliases for non-debug version.
 * -> This way we can keep binary compatibility between all versions of libc. */
__public __crit void *_malloc_d(size_t s __LIBC_DEBUG__UPARAMS) { return malloc(s); }
__public __crit void *_calloc_d(size_t n, size_t s __LIBC_DEBUG__UPARAMS) { return calloc(n,s); }
__public __crit void *_realloc_d(void *__restrict p, size_t s __LIBC_DEBUG__UPARAMS) { return realloc(p,s); }
__public __crit void *_memalign_d(size_t alignment, size_t bytes __LIBC_DEBUG__UPARAMS) { return memalign(alignment,bytes); }
__public __crit void _free_d(void *__restrict p __LIBC_DEBUG__UPARAMS) { free(p); }
__public __crit char *_strdup_d(char const *__restrict s __LIBC_DEBUG__UPARAMS) { return strdup(s); }
__public __crit char *_strndup_d(char const *__restrict s, size_t maxchars __LIBC_DEBUG__UPARAMS) { return strndup(s,maxchars); }
__public __crit void *_memdup_d(void const *__restrict p, size_t bytes __LIBC_DEBUG__UPARAMS) { return _memdup(p,bytes); }
__public __crit size_t _malloc_usable_size_d(void *__restrict p __LIBC_DEBUG__UPARAMS) { return malloc_usable_size(p); }
__public __crit int _posix_memalign_d(void **__restrict memptr, size_t alignment, size_t bytes __LIBC_DEBUG__UPARAMS) { return posix_memalign(memptr,alignment,bytes); }
__public __crit void *_pvalloc_d(size_t bytes __LIBC_DEBUG__UPARAMS) { return pvalloc(bytes); }
__public __crit void *_valloc_d(size_t bytes __LIBC_DEBUG__UPARAMS) { return valloc(bytes); }
__public __crit char *_strdupf_d(__LIBC_DEBUG_UPARAMS_ char const *__restrict format, ...) { char *result; va_list args; va_start(args,format); result = _vstrdupf(format,args); va_end(args); return result; }
__public __crit char *_vstrdupf_d(char const *__restrict format, va_list args __LIBC_DEBUG__UPARAMS) { return _vstrdupf(format,args); }
__public void *__mallblock_getattrib_d(struct _mallblock_d *__restrict __unused(__self), int __unused(attrib)) { return NULL; }
__public int _mallblock_traceback_d(struct _mallblock_d *__restrict __unused(self), _ptraceback_stackwalker_d __unused(callback), void *__unused(closure)) { return 0; }
__public int _malloc_enumblocks_d(void *__unused(checkpoint), int (*callback)(struct _mallblock_d *__restrict block, void *closure), void *__unused(closure)) { (void)callback; return 0; }
__public void _malloc_printleaks_d(void) {}
__public void _malloc_validate_d(void) {}


/* Non-debug implementations of malloc helpers. */
__public __crit void *_memdup(void const *__restrict p, size_t bytes) {
 void *result = malloc(bytes);
 if __likely(result) memcpy(result,p,bytes);
 return result;
}
__public __crit char *strdup(char const *__restrict s) {
 return (char *)_memdup(s,(strlen(s)+1)*sizeof(char));
}
__public __crit char *strndup(char const *__restrict s, size_t maxchars) {
 size_t copy_chars = strnlen(s,maxchars);
 char *result = (char *)malloc((copy_chars+1)*sizeof(char));
 if __likely(result) {
  memcpy(result,s,copy_chars);
  result[copy_chars] = '\0';
 }
 return result;
}
__public __crit char *_strdupf(char const *__restrict format, ...) {
 va_list args; char *result;
 va_start(args,format);
 result = _vstrdupf(format,args);
 va_end(args);
 return result;
}

struct strdup_formatdata { char *start,*iter,*end; };

static int
strdupf_printer(char const *__restrict data, size_t maxchars,
                struct strdup_formatdata *__restrict fmt) {
 char *newiter;
 maxchars = strnlen(data,maxchars);
 newiter = fmt->iter+maxchars;
 if (newiter > fmt->end) {
  size_t newsize = (size_t)(fmt->end-fmt->start);
  assert(newsize);
  do newsize *= 2;
  while (fmt->start+newsize < newiter);
  /* Realloc the strdup string */
  newiter = (char *)realloc(fmt->start,(newsize+1)*sizeof(char));
  if __unlikely(!newiter) {
   /* If there isn't enough memory, retry
    * with a smaller buffer before giving up. */
   newsize = (fmt->end-fmt->start)+maxchars;
   newiter = (char *)realloc(fmt->start,(newsize+1)*sizeof(char));
   if __unlikely(!newiter) return -1; /* Nothing we can do (out of memory...) */
  }
  fmt->iter = newiter+(fmt->iter-fmt->start);
  fmt->start = newiter;
  fmt->end = newiter+newsize;
 }
 memcpy(fmt->iter,data,maxchars);
 fmt->iter += maxchars;
 return 0;
}


__public __crit char *
_vstrdupf(char const *__restrict format,
          va_list args) {
 struct strdup_formatdata data;
 /* Try to do a (admittedly very bad) prediction on the required size. */
 size_t format_length = (strlen(format)*3)/2;
 data.start = (char *)malloc((format_length+1)*sizeof(char));
 if __unlikely(!data.start) {
  /* Failed to allocate initial buffer (try with a smaller one) */
  format_length = 1;
  data.start = (char *)malloc(2*sizeof(char));
  if __unlikely(!data.start) return NULL;
 }
 data.end = data.start+format_length;
 data.iter = data.start;
 if __unlikely(format_vprintf((pformatprinter)&strdupf_printer,
                              &data,format,args) != 0) {
  free(data.start); /* Out-of-memory */
  return NULL;
 }
 *data.iter = '\0';
 if __likely(data.iter != data.end) {
  /* Try to realloc the string one last time to save up on memory */
  data.end = (char *)realloc(data.start,((data.iter-data.start)+1)*sizeof(char));
  if __likely(data.end) data.start = data.end;
 }
 return data.start;
}
#endif

#ifndef __KERNEL__
#undef cfree
__public __compiler_ALIAS(cfree,free);
#undef aligned_alloc
__public __compiler_ALIAS(aligned_alloc,memalign);
#endif /* !__KERNEL__ */

static int __printfsize_callback(char const *data, size_t maxlen, size_t *result) {
 *result += strnlen(data,maxlen);
 return 0;
}
__local size_t __printfsize(char const *fmt, va_list args) {
 size_t result = 1; /*< +1 for ZERO-termination. */
 format_vprintf((pformatprinter)&__printfsize_callback,
                &result,fmt,args);
 return result;
}

#undef _vstrdupaf
__public char *__cdecl _vstrdupaf(char const *fmt, va_list args) {
 /* For a more detailed explanation of
  * what happens here, see '_strdupaf'. */
 size_t reqsize; char *result;
 va_list args2;
 va_copy(args2,args);
 reqsize = __printfsize(fmt,args2);
 va_end(args2);
 result = (char *)alloca(reqsize);
 vsprintf(result,fmt,args);
 /* We waste some memory by not cleaning up arguments...
  * An explanation why we're still not doing it here,
  * even though we can actually figure out the correct
  * ESP to offset by, can be found below
  * (basically: The memmove overhead isn't worth it) */
 __asm_volatile__("mov %0, %%eax\n"
                  "jmp __strdupaf_end\n"
                  : : "g" (result));
 __builtin_unreachable();
}

#undef _strdupaf
__public char *__cdecl _strdupaf(char const *fmt, ...) {
 size_t reqsize; char *result; va_list args;
 /* Since we need the exact size beforehand, and since
  * we can't realloc alloca-ted memory, we must sadly
  * format_printf the given formatter twice. */
 va_start(args,fmt);
 reqsize = __printfsize(fmt,args);
 va_end(args);
 /* My calling alloca here, we force the compiler to generate
  * a real stackframe for this function, a fact we rely upon
  * in the custom return code below. */
 result = (char *)alloca(reqsize);
 /* Do the second format-printf (this time write to the given buffer).
  * No need to use vnsprintf, because the buffer is fitted to the exact
  * size of the format string to-be generated. */
 va_start(args,fmt);
 vsprintf(result,fmt,args);
 va_end(args);
 /* We waste a lot of memory by not cleaning up arguments...
  * In theory, we could try to figure out how much memory is
  * used my the arguments by looking at the format string,
  * them do a memmove, shifting the generated printf-string
  * downwards to overwrite the memory previously used by them.
  * Then we effectively return a pointer to a zero-terminated string,
  * who's strend() is equal to the address of the last va_list argument.
  * 
  * But that is really the most impractical thing I could even
  * think about, not just because of the additional overhead
  * of having to move the entirely of the newly printed string
  * just after we created it (meaning this function would be
  * much slower), but also because we're talking the stack here!
  * I mean come on: Overflowing the stack is undefined behavior
  *                 galore, and whats +- a few bytes going to do?
  * But seriously though: This wouldn't be worth it.
  * 
  * You also must consider what contexts strdupaf is meant to
  * be used in, being designed for things like concat-ing small
  * string together without the overhead of malloc()-ing new memory,
  * as well as the hassle of having to free() it afterwards.
  * 
  * REMEMBER: Because the stack grows downwards, 'fmt' is actually
  *           the last argument in an alloca-context, as it has the
  *           lowest memory address of all.
  */
 __asm_volatile__("mov %0, %%eax\n"
"__strdupaf_end:\n"
                  /* Extract the return address and retore the original %EBP. */
                  "mov 4(%%ebp), %%ebx\n"
                  "mov 0(%%ebp), %%ebp\n"
                  /* Push the return address to have ret return to the caller. */
                  "push %%ebx\n"
                  /* By never restoring %ESP, we've effectively passed
                   * the allocated memory onwards to the caller. */
                  "ret\n"
                  : : "g" (result));
 __builtin_unreachable();
}


__DECL_END



#if defined(_MSC_VER) || defined(__INTELLISENSE__)
#include <alloca.h>
__DECL_BEGIN

#if defined(__LIBC_MUST_IMPLEMENT_LIBC_ALLOCA) || !defined(__KERNEL__)
/* Alloca is important enough to already inquire a MSVC implementation
 * (Plus: I was bored) */
#ifndef __KERNEL__
#undef alloca
__public __declspec(naked)
extern void *alloca(size_t size) {
 __asm {
  pop  ebx        /* Return address */
  mov  eax, esp   /* Store old ESP in eax */
  sub  eax, [esp] /* Size to allocate (subtract from old ESP) */
  mov  esp, eax   /* Store new ESP. */
  push ebx        /* Push return address. */
  ret
 }
}
__public __declspec(naked)
extern void *__libc_alloca(size_t size) {
 /* Ugh... (twice?) */
 __asm {
  pop  ebx        /* Return address */
  mov  eax, esp   /* Store old ESP in eax */
  sub  eax, [esp] /* Size to allocate (subtract from old ESP) */
  mov  esp, eax   /* Store new ESP. */
  push ebx        /* Push return address. */
  ret
 }
}
#endif
#endif

__DECL_END
#endif

#endif /* !__MALLOC_C__ */
