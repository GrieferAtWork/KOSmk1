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
#ifndef __KOS_SYSCALLNO_H__
#define __KOS_SYSCALLNO_H__ 1

#include <kos/compiler.h>

__DECL_BEGIN

#define SYS_k_syslog      0 /*< _syscall3(kerrno_t,k_syslog,int,level,char const *,s,size_t,maxlen); */
#define SYS_k_sysconf     1 /*< _syscall1(long,k_sysconf,int,name); */

#define SYS_kfd_open      2   /*< _syscall5(kerrno_t,kfd_open,int,cwd,char const *,path,size_t,maxpath,openmode_t,mode,mode_t,perms); */
#define SYS_kfd_open2     3   /*< _syscall6(kerrno_t,kfd_open2,int,dfd,int,cwd,char const *,path,size_t,maxpath,openmode_t,mode,mode_t,perms); */
#define SYS_kfd_close     4   /*< _syscall1(kerrno_t,kfd_close,int,fd); */
#define SYS_kfd_closeall  5   /*< _syscall2(unsigned int,kfd_close,int,low,int,high); */
#define SYS_kfd_seek      6   /*< _syscall{4|5}(kerrno_t,kfd_seek,int,fd,{__s64,off|__s32,offhi,__s32,offlo},int,whence,__u64 *,newpos); */
#define SYS_kfd_read      7   /*< _syscall4(kerrno_t,kfd_read,int,fd,void *,buf,size_t,bufsize,size_t *,rsize); */
#define SYS_kfd_write     8   /*< _syscall4(kerrno_t,kfd_write,int,fd,void const *,buf,size_t,bufsize,size_t *,wsize); */
#define SYS_kfd_pread     9   /*< _syscall{5|6}(kerrno_t,kfd_pread,int,fd,{__u64,pos|__u32,poshi,__u32,poslo},void *,buf,size_t,bufsize,size_t *,rsize); */
#define SYS_kfd_pwrite    10  /*< _syscall{5|6}(kerrno_t,kfd_pwrite,int,fd,{__u64,pos|__u32,poshi,__u32,poslo},void const *,buf,size_t,bufsize,size_t *,wsize); */
#define SYS_kfd_flush     11  /*< _syscall1(kerrno_t,kfd_flush,int,fd); */
#define SYS_kfd_trunc     12  /*< _syscall{2|3}(kerrno_t,kfd_trunc,int,fd,{__u64,size|__u32,sizehi,__u32,sizelo}); */
#define SYS_kfd_fcntl     13  /*< _syscall3(kerrno_t,kfd_fcntl,int,fd,int,cmd,void *,arg); */
#define SYS_kfd_ioctl     14  /*< _syscall3(kerrno_t,kfd_ioctl,int,fd,kattr_t,attr,void *,arg); */
#define SYS_kfd_getattr   15  /*< _syscall5(kerrno_t,kfd_getattr,int,fd,kattr_t,attr,void *,buf,size_t,bufsize,size_t *,reqsize); */
#define SYS_kfd_setattr   16  /*< _syscall4(kerrno_t,kfd_setattr,int,fd,kattr_t,attr,void const *,buf,size_t,bufsize); */
#define SYS_kfd_readdir   17  /*< _syscall3(kerrno_t,kfd_readdir,int,fd,struct kfddirent *,dent,__u32,flags); */
#define SYS_kfd_readlink  18  /*< _syscall4(kerrno_t,kfd_readlink,int,fd,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
#define SYS_kfd_dup       19  /*< _syscall2(int,kfd_dup,int,fd,int,flags); */
#define SYS_kfd_dup2      20  /*< _syscall3(int,kfd_dup2,int,fd,int,resfd,int,flags); */
#define SYS_kfd_pipe      21  /*< _syscall3(kerrno_t,kfd_pipe,int *,pipefd,int,flags,__size_t,max_size); */
#define SYS_kfd_equals    22  /*< _syscall2(int,kfd_equals,int,fda,int,fdb); */
#define SYS_kfd_openpty   23  /*< _syscall5(kerrno_t,kfd_openpty,int *,amaster,int *,aslave,char *,name,struct termios const *,termp,struct winsize const *,winp); */

#define SYS_kfs_mkdir     24 /*< _syscall4(kerrno_t,kfs_mkdir,int,dirfd,char const *,path,size_t,pathmax,mode_t,mode); */
#define SYS_kfs_rmdir     25 /*< _syscall3(kerrno_t,kfs_rmdir,int,dirfd,char const *,path,size_t,pathmax); */
#define SYS_kfs_unlink    26 /*< _syscall3(kerrno_t,kfs_unlink,int,dirfd,char const *,path,size_t,pathmax); */
#define SYS_kfs_remove    27 /*< _syscall3(kerrno_t,kfs_remove,int,dirfd,char const *,path,size_t,pathmax); */
#define SYS_kfs_symlink   28 /*< _syscall5(kerrno_t,kfs_symlink,int,dirfd,char const *,target,size_t,targetmax,char const *,lnk,size_t,lnkmax); */
#define SYS_kfs_hrdlink   29 /*< _syscall5(kerrno_t,kfs_hrdlink,int,dirfd,char const *,target,size_t,targetmax,char const *,lnk,size_t,lnkmax); */

#define SYS_ktime_getnow  30 /*< _syscall1(kerrno_t,ktime_getnow,struct timespec *,ptm); */
#define SYS_ktime_setnow  31 /*< _syscall1(kerrno_t,ktime_setnow,struct timespec *,ptm); */
#define SYS_ktime_htick   32 /*< {_syscall0(__u64,ktime_htick)|special(lo:eax;hi:ecx)}; */
#define SYS_ktime_hfreq   88 /*< {_syscall0(__u64,ktime_hfreq)|special(lo:eax;hi:ecx)}; */

#define SYS_kmem_map      33 /*< _syscall6(void *,kmem_map,void *,hint,size_t,length,int,prot,int,flags,int,fd,__u64,offset); */
#define SYS_kmem_unmap    34 /*< _syscall2(kerrno_t,kmem_unmap,void *,addr,size_t,length); */
#define SYS_kmem_validate 35 /*< _syscall2(kerrno_t,kmem_validate,void *__restrict,addr,size_t,bytes); */
#define SYS_kmem_mapdev   36 /*< _syscall5(kerrno_t,kmem_mapdev,void **,hint_and_result,__size_t,length,int,prot,int,flags,void *,physptr); */

#define SYS_ktask_yield        37 /*< _syscall0(kerrno_t,ktask_yield); */
#define SYS_ktask_setalarm     38 /*< _syscall3(kerrno_t,ktask_setalarm,int,self,struct timespec const *__restrict,abstime,struct timespec *__restrict,oldabstime); */
#define SYS_ktask_getalarm     39 /*< _syscall2(kerrno_t,ktask_getalarm,int,self,struct timespec *__restrict,abstime); */
#define SYS_ktask_abssleep     40 /*< _syscall2(kerrno_t,ktask_abssleep,int,self,struct timespec const *__restrict,abstime); */
#define SYS_ktask_terminate    41 /*< _syscall3(kerrno_t,ktask_terminate,int,self,void *,exitcode,ktaskopflag_t,flags); */
#define SYS_ktask_suspend      42 /*< _syscall2(kerrno_t,ktask_suspend,int,self,ktaskopflag_t,flags); */
#define SYS_ktask_resume       43 /*< _syscall2(kerrno_t,ktask_resume,int,self,ktaskopflag_t,flags); */
#define SYS_ktask_openroot     44 /*< _syscall1(int,ktask_openroot,int,self); */
#define SYS_ktask_openparent   45 /*< _syscall1(int,ktask_openparent,int,self); */
#define SYS_ktask_openproc     46 /*< _syscall1(int,ktask_openproc,int,self); */
#define SYS_ktask_getparid     47 /*< _syscall1(size_t,ktask_getparid,int,self); */
#define SYS_ktask_gettid       48 /*< _syscall1(ktid_t,ktask_gettid,int,self); */
#define SYS_ktask_openchild    49 /*< _syscall2(int,ktask_openchild,int,self,size_t,parid); */
#define SYS_ktask_enumchildren 50 /*< _syscall5(kerrno_t,ktask_enumchildren,int,self,size_t *__restrict,idv,size_t,idc,size_t *,reqidc,ktaskopflag_t,flags); */
#define SYS_ktask_getpriority  51 /*< _syscall2(kerrno_t,ktask_getpriority,int,self,ktaskprio_t *__restrict,result); */
#define SYS_ktask_setpriority  52 /*< _syscall2(kerrno_t,ktask_setpriority,int,self,ktaskprio_t,value); */
#define SYS_ktask_join         53 /*< _syscall3(kerrno_t,ktask_join,int,self,void **__restrict,exitcode,__u32,pending_argument); */
#define SYS_ktask_tryjoin      54 /*< _syscall3(kerrno_t,ktask_tryjoin,int,self,void **__restrict,exitcode,__u32,pending_argument); */
#define SYS_ktask_timedjoin    55 /*< _syscall4(kerrno_t,ktask_timedjoin,int,self,struct timespec const *__restrict,abstime,void **__restrict,exitcode,__u32,pending_argument); */
#define SYS_ktask_newthread    56 /*< _syscall4(int,ktask_newthread,ktask_threadfun_t,thread_main,void *,closure,__u32,flags,void **,arg); */
#define SYS_ktask_newthreadi   57 /*< _syscall5(int,ktask_newthreadi,ktask_threadfun_t,thread_main,void const *,buf,size_t,bufsize,__u32,flags,void **,arg); */
#define SYS_ktask_fork         58 /*< _syscall2(kerrno_t,ktask_fork,uintptr_t *,childfd_or_exitcode,__u32,flags); */
#define SYS_ktask_exec         59 /*< _syscall4(kerrno_t,ktask_exec,char const *,path,__size_t,pathmax,struct kexecargs const *,args,__u32,flags); */
#define SYS_ktask_fexec        60 /*< _syscall3(kerrno_t,ktask_fexec,int,fd,struct kexecargs const *,args,__u32,flags); */

#define SYS_kproc_enumfd       65  /*< _syscall4(kerrno_t,kproc_enumfd,int,self,int *__restrict,fdv,size_t,fdc,size_t *,reqfdc); */
#define SYS_kproc_openfd       66  /*< _syscall3(int,kproc_openfd,int,self,int,fd,int,flags); */
#define SYS_kproc_openfd2      67  /*< _syscall4(int,kproc_openfd2,int,self,int,fd,int,newfd,int,flags); */
#define SYS_kproc_barrier      68  /*< _syscall1(kerrno_t,kproc_barrier,ksandbarrier_t,level); */
#define SYS_kproc_openbarrier  69  /*< _syscall2(int,kproc_openbarrier,int,self,ksandbarrier_t,level); */
#define SYS_kproc_openroot     SYS_ktask_openroot               /*< _syscall1(int,kproc_openroot,int,self); */
#define SYS_kproc_openthread   SYS_ktask_openchild              /*< _syscall2(int,kproc_openthread,int,self,ktid_t,tid); */
#define SYS_kproc_enumthreads  SYS_ktask_enumchildren           /*< _syscall4(kerrno_t,kproc_enumthreads,int,self,ktid_t *__restrict,idv,size_t,idc,size_t *,reqidc); */
#define SYS_kproc_tlsalloc     70 /*< _syscall2(kerrno_t,kproc_tlsalloc,void const *,template_,size_t,template_size) && return:(ecx=tls_offset); */
#define SYS_kproc_tlsfree      71 /*< _syscall1(kerrno_t,kproc_tlsfree,__ptrdiff_t,tls_offset); */
#define SYS_kproc_tlsenum      72 /*< _syscall2(kerrno_t,kproc_tlsenum,struct ktlsinfo *__restrict,infov,__size_t,infoc) && return:(ecx=reqinfoc); */
#define SYS_kproc_enumpid      73 /*< _syscall3(kerrno_t,kproc_enumpid,__pid_t *__restrict,pidv,size_t,pidc,size_t *,reqpidc); */
#define SYS_kproc_openpid      74 /*< _syscall1(int,kproc_openpid,__pid_t,pid); */
#define SYS_kproc_getpid       75 /*< _syscall1(__pid_t,kproc_getpid,int,self); */
#define SYS_kproc_getenv       76 /*< _syscall6(kerrno_t,kproc_getenv,int,self,char const *,name,__size_t,namemax,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
#define SYS_kproc_setenv       77 /*< _syscall6(kerrno_t,kproc_setenv,int,self,char const *,name,__size_t,namemax,char const *,value,__size_t,valuemax,int,override); */
#define SYS_kproc_delenv       78 /*< _syscall3(kerrno_t,kproc_delenv,int,self,char const *,name,__size_t,namemax); */
#define SYS_kproc_enumenv      79 /*< _syscall4(kerrno_t,kproc_enumenv,int,self,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
#define SYS_kproc_getcmd       80 /*< _syscall4(kerrno_t,kproc_getcmd,int,self,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
#define SYS_kproc_perm         81 /*< _syscall4(kerrno_t,kproc_perm,int,procfd,struct kperm *,buf,__size_t,elem_count,int,mode); */

// The following set of system calls are mere aliases for their task-counterparts.
// >> On order to function like one would expect from these
//   (e.g.: 'kproc_terminate' terminating not just the given, but all threads),
//    when suitable, the 'KTASKOPFLAG_RECURSIVE' flag must be set.
// WARNING: Currently 'kproc_join' will only wait until the process's root
//          thread has terminated (aka. you 'main()' function returns).
//          If at that point more threads are still running, they 
//          continue to, even though control has already returned to you.
//       >> In the future, the 'pending_argument' flag for 'ktask_join'
//          is probably going to be used as a hint to also wait for all
//          child tasks to terminate as well, though currently its unused.
// WARNING: 'kproc_(g|s)etpriority' only affects with the root task of the process.
#define SYS_kproc_terminate   SYS_ktask_terminate              /*< _syscall3(kerrno_t,kproc_terminate,int,self,void *,exitcode,ktaskopflag_t,flags); */
#define SYS_kproc_suspend     SYS_ktask_suspend                /*< _syscall2(kerrno_t,kproc_suspend,int,self,ktaskopflag_t,flags); */
#define SYS_kproc_resume      SYS_ktask_resume                 /*< _syscall2(kerrno_t,kproc_resume,int,self,ktaskopflag_t,flags); */
#define SYS_kproc_join        SYS_ktask_join                   /*< _syscall3(kerrno_t,kproc_join,int,self,void **__restrict,exitcode,__u32,pending_argument); */
#define SYS_kproc_tryjoin     SYS_ktask_tryjoin                /*< _syscall3(kerrno_t,kproc_tryjoin,int,self,void **__restrict,exitcode,__u32,pending_argument); */
#define SYS_kproc_timedjoin   SYS_ktask_timedjoin              /*< _syscall4(kerrno_t,kproc_timedjoin,int,self,struct timespec const *__restrict,abstime,void **__restrict,exitcode,__u32,pending_argument); */
#define SYS_kproc_getpriority SYS_ktask_getpriority            /*< _syscall2(kerrno_t,kproc_getpriority,int,self,ktaskprio_t *__restrict,result); */
#define SYS_kproc_setpriority SYS_ktask_setpriority            /*< _syscall2(kerrno_t,kproc_setpriority,int,self,ktaskprio_t,value); */

// v TODO: [!SYS_kmod_sym] add 'int self' (procfd) to these syscalls & add 'kmod_enum'
#define SYS_kmod_open    82 /* _syscall4(kerrno_t,kmod_open,char const *,name,size_t,namemax,kmodid_t *,modid,__u32,flags); */
#define SYS_kmod_fopen   83 /* _syscall3(kerrno_t,kmod_open,int,fd,kmodid_t *,modid,__u32,flags); */
#define SYS_kmod_close   84 /* _syscall1(kerrno_t,kmod_close,kmodid_t,modid); */
#define SYS_kmod_sym     85 /* _syscall3(void *,kmod_sym,kmodid_t,modid,char const *,name,size_t,namemax); */
#define SYS_kmod_info    86 /* _syscall6(kerrno_t,kmod_info,int,procfd,kmodid_t,modid,struct kmodinfo *,buf,size_t,bufsize,size_t *,reqsize,__u32,flags); */
#define SYS_kmod_syminfo 87 /* _syscall7(kerrno_t,kmod_syminfo,int,procfd,kmodid_t,modid,void const *,addr_or_name,struct ksyminfo *,buf,size_t,bufsize,size_t *,reqsize,__u32,flags); */

__DECL_END

#endif /* !__KOS_SYSCALLNO_H__ */
