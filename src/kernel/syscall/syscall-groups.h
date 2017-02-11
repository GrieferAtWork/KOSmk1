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

// syscall-kgeneric.c.inl
GROUPBEGIN(sys_generic)
  ENTRY(k_syslog)     /*< _syscall2(kerrno_t,k_syslog,char const *,s,size_t,maxlen); */
  ENTRY(k_sysconf) /*< _syscall1(long,k_sysconf,int,name); */
GROUPEND

// syscall-kfd.c.inl
GROUPBEGIN(sys_kfd)
  ENTRY(kfd_open)     /*< _syscall5(kerrno_t,kfd_open,int,cwd,char const *,path,size_t,maxpath,openmode_t,mode,mode_t,perms); */
  ENTRY(kfd_open2)    /*< _syscall6(kerrno_t,kfd_open2,int,dfd,int,cwd,char const *,path,size_t,maxpath,openmode_t,mode,mode_t,perms); */
  ENTRY(kfd_close)    /*< _syscall1(kerrno_t,kfd_close,int,fd); */
  ENTRY(kfd_closeall) /*< _syscall2(unsigned int,kfd_close,int,low,int,high); */
  ENTRY(kfd_seek)     /*< _syscall{4|5}(kerrno_t,kfd_seek,int,fd,{__s64,off|__s32,offhi,__s32,offlo},int,whence,__u64 *,newpos); */
  ENTRY(kfd_read)     /*< _syscall4(kerrno_t,kfd_read,int,fd,void *,buf,size_t,bufsize,size_t *,rsize); */
  ENTRY(kfd_write)    /*< _syscall4(kerrno_t,kfd_write,int,fd,void const *,buf,size_t,bufsize,size_t *,wsize); */
  ENTRY(kfd_pread)    /*< _syscall{5|6}(kerrno_t,kfd_pread,int,fd,{__u64,pos|__u32,poshi,__u32,poslo},void *,buf,size_t,bufsize,size_t *,rsize); */
  ENTRY(kfd_pwrite)   /*< _syscall{5|6}(kerrno_t,kfd_pwrite,int,fd,{__u64,pos|__u32,poshi,__u32,poslo},void const *,buf,size_t,bufsize,size_t *,wsize); */
  ENTRY(kfd_flush)    /*< _syscall1(kerrno_t,kfd_flush,int,fd); */
  ENTRY(kfd_trunc)    /*< _syscall{2|3}(kerrno_t,kfd_trunc,int,fd,{__u64,size|__u32,sizehi,__u32,sizelo}); */
  ENTRY(kfd_fcntl)    /*< _syscall3(kerrno_t,kfd_fcntl,int,fd,int,cmd,void *,arg); */
  ENTRY(kfd_ioctl)    /*< _syscall3(kerrno_t,kfd_ioctl,int,fd,kattr_t,attr,void *,cmd); */
  ENTRY(kfd_getattr)  /*< _syscall5(kerrno_t,kfd_getattr,int,fd,kattr_t,attr,void *,buf,size_t,bufsize,size_t *,reqsize); */
  ENTRY(kfd_setattr)  /*< _syscall4(kerrno_t,kfd_setattr,int,fd,kattr_t,attr,void const *,buf,size_t,bufsize); */
  ENTRY(kfd_readdir)  /*< _syscall3(kerrno_t,kfd_readdir,int,fd,struct kfddirent *,dent,__u32,flags); */
  ENTRY(kfd_readlink) /*< _syscall4(kerrno_t,kfd_readlink,int,fd,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
  ENTRY(kfd_dup)      /*< _syscall2(int,kfd_dup,int,fd,int,flags); */
  ENTRY(kfd_dup2)     /*< _syscall3(int,kfd_dup2,int,fd,int,resfd,int,flags); */
  ENTRY(kfd_pipe)     /*< _syscall2(kerrno_t,kfd_pipe,int *,pipefd,int,flags); */
  ENTRY(kfd_equals)   /*< _syscall2(kerrno_t,kfd_equals,int,fda,int,fdb); */
  ENTRY(kfd_openpty)  /*< _syscall5(kerrno_t,kfd_openpty,int *,amaster,int *,aslave,char *,name,struct termios const *,termp,struct winsize const *,winp); */
GROUPEND

// syscall-kfs.c.inl
GROUPBEGIN(sys_kfs)
  ENTRY(kfs_mkdir)   /*< _syscall4(kerrno_t,kfs_mkdir,int,dirfd,char const *,path,size_t,pathmax,mode_t,perms); */
  ENTRY(kfs_rmdir)   /*< _syscall3(kerrno_t,kfs_rmdir,int,dirfd,char const *,path,size_t,pathmax); */
  ENTRY(kfs_unlink)  /*< _syscall3(kerrno_t,kfs_unlink,int,dirfd,char const *,path,size_t,pathmax); */
  ENTRY(kfs_remove)  /*< _syscall3(kerrno_t,kfs_remove,int,dirfd,char const *,path,size_t,pathmax); */
  ENTRY(kfs_symlink) /*< _syscall5(kerrno_t,kfs_symlink,int,dirfd,char const *,target,size_t,targetmax,char const *,lnk,size_t,lnkmax); */
  ENTRY(kfs_hrdlink) /*< _syscall5(kerrno_t,kfs_hrdlink,int,dirfd,char const *,target,size_t,targetmax,char const *,lnk,size_t,lnkmax); */
GROUPEND

// syscall-ktime.c.inl
GROUPBEGIN(sys_ktm)
  ENTRY(ktime_getnow) /*< _syscall1(kerrno_t,ktime_getnow,struct timespec *,ptm); */
  ENTRY(ktime_setnow) /*< _syscall1(kerrno_t,ktime_setnow,struct timespec *,ptm); */
  ENTRY(ktime_getcpu) /*< _syscall1(kerrno_t,ktime_getcpu,struct timespec *,ptm); */
GROUPEND

// syscall-kmem.c.inl
GROUPBEGIN(sys_kmem)
  ENTRY(kmem_map)      /*< _syscall6(void *,kmem_map,void *,hint,size_t,length,int,prot,int,flags,int,fd,__u64,offset); */
  ENTRY(kmem_unmap)    /*< _syscall2(kerrno_t,kmem_unmap,void *,addr,size_t,length); */
  ENTRY(kmem_validate) /*< _syscall2(kerrno_t,kmem_validate,void const *__restrict,addr,__size_t,bytes); */
  ENTRY(kmem_mapdev)   /*< _syscall5(kerrno_t,kmem_mapdev,void **,hint_and_result,__size_t,length,int,prot,int,flags,void *,physptr); */
GROUPEND

// syscall-ktask.c.inl
GROUPBEGIN(sys_ktask)
  ENTRY(ktask_yield)        /*< _syscall0(kerrno_t,ktask_yield); */
  ENTRY(ktask_setalarm)     /*< _syscall3(kerrno_t,ktask_setalarm,int,task,struct timespec const *__restrict,abstime,struct timespec *__restrict,oldabstime); */
  ENTRY(ktask_getalarm)     /*< _syscall2(kerrno_t,ktask_getalarm,int,task,struct timespec *__restrict,abstime); */
  ENTRY(ktask_abssleep)     /*< _syscall2(kerrno_t,ktask_abssleep,int,task,struct timespec const *__restrict,timeout); */
  ENTRY(ktask_terminate)    /*< _syscall3(kerrno_t,ktask_terminate,int,task,void *,exitcode,ktaskopflag_t,flags); */
  ENTRY(ktask_suspend)      /*< _syscall2(kerrno_t,ktask_suspend,int,task,ktaskopflag_t,flags); */
  ENTRY(ktask_resume)       /*< _syscall2(kerrno_t,ktask_resume,int,task,ktaskopflag_t,flags); */
  ENTRY(ktask_openroot)     /*< _syscall1(int,ktask_openroot,int,task); */
  ENTRY(ktask_openparent)   /*< _syscall1(int,ktask_openparent,int,self); */
  ENTRY(ktask_openproc)     /*< _syscall1(int,ktask_openproc,int,self); */
  ENTRY(ktask_getparid)     /*< _syscall1(size_t,ktask_getparid,int,self); */
  ENTRY(ktask_gettid)       /*< _syscall1(ktid_t,ktask_gettid,int,self); */
  ENTRY(ktask_openchild)    /*< _syscall2(int,ktask_openchild,int,self,size_t,parid); */
  ENTRY(ktask_enumchildren) /*< _syscall4(kerrno_t,ktask_enumchildren,int,self,size_t *__restrict,idv,size_t,idc,size_t *,reqidc); */
  ENTRY(ktask_getpriority)  /*< _syscall2(kerrno_t,ktask_getpriority,int,self,ktaskprio_t *__restrict,result); */
  ENTRY(ktask_setpriority)  /*< _syscall2(kerrno_t,ktask_setpriority,int,self,ktaskprio_t,value); */
  ENTRY(ktask_join)         /*< _syscall2(kerrno_t,ktask_join,int,self,void **__restrict,exitcode); */
  ENTRY(ktask_tryjoin)      /*< _syscall2(kerrno_t,ktask_tryjoin,int,self,void **__restrict,exitcode); */
  ENTRY(ktask_timedjoin)    /*< _syscall3(kerrno_t,ktask_timedjoin,int,self,struct timespec const *__restrict,abstime,void **__restrict,exitcode); */
  ENTRY(ktask_newthread)    /*< _syscall4(int,ktask_newthread,ktask_threadfunc,thread_main,void *,closure,__u32,flags,void **,arg); */
  ENTRY(ktask_newthreadi)   /*< _syscall5(int,ktask_newthreadi,ktask_threadfunc,thread_main,void const *,buf,size_t,bufsize,__u32,flags,void **,arg); */
  ENTRY(ktask_fork)         /*< _syscall2(kerrno_t,ktask_fork,uintptr_t *,childfd_or_exitcode,__u32,flags); */
  ENTRY(ktask_exec)         /*< _syscall6(kerrno_t,ktask_exec,char const *,path,size_t,pathmax,size_t,argc,char const *const *,argv,char const *const *,envp,__u32,flags); */
  ENTRY(ktask_fexec)        /*< _syscall5(kerrno_t,ktask_exec,int,fd,size_t,argc,char const *const *,argv,char const *const *,envp,__u32,flags); */
  ENTRY(ktask_gettls)       /*< _syscall1(void *,ktask_gettls,ktls_t,slot); */
  ENTRY(ktask_settls)       /*< _syscall3(kerrno_t,ktask_settls,ktls_t,slot,void *,value,void **,oldvalue); */
  ENTRY(ktask_gettlsof)     /*< _syscall2(void *,ktask_gettls,int,self,ktls_t,slot); */
  ENTRY(ktask_settlsof)     /*< _syscall4(kerrno_t,ktask_settls,int,self,ktls_t,slot,void *,value,void **,oldvalue); */
GROUPEND

GROUPBEGIN(sys_kproc)
  ENTRY(kproc_enumfd)      /*< _syscall4(kerrno_t,kproc_enumfd,int,self,int *__restrict,fdv,size_t,fdc,size_t *,reqfdc); */
  ENTRY(kproc_openfd)      /*< _syscall3(int,kproc_openfd,int,self,int,fd,int,flags); */
  ENTRY(kproc_openfd2)     /*< _syscall4(int,kproc_openfd2,int,self,int,fd,int,newfd,int,flags); */
  ENTRY(kproc_barrier)     /*< _syscall1(kerrno_t,kproc_barrier,ksandbarrier_t,level); */
  ENTRY(kproc_openbarrier) /*< _syscall2(int,kproc_openbarrier,int,self,ksandbarrier_t,level); */
  ENTRY(kproc_alloctls)    /*< _syscall1(kerrno_t,kproc_alloctls_c,ktls_t *,result); */
  ENTRY(kproc_freetls)     /*< _syscall1(kerrno_t,kproc_freetls_c,ktls_t,slot); */
  ENTRY(kproc_enumtls)     /*< _syscall4(kerrno_t,kproc_enumtls,int,self,ktls_t *__restrict,tlsv,size_t,tlsc,size_t *,reqtlsc); */
  ENTRY(kproc_enumpid)     /*< _syscall3(kerrno_t,kproc_enumpid,__pid_t *__restrict,pidv,size_t,pidc,size_t *,reqpidc); */
  ENTRY(kproc_openpid)     /*< _syscall1(int,kproc_openpid,__pid_t,pid); */
  ENTRY(kproc_getpid)      /*< _syscall1(__pid_t,kproc_getpid,int,self); */
  ENTRY(kproc_getenv)      /*< _syscall6(kerrno_t,kproc_getenv,int,self,char const *,name,__size_t,namemax,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
  ENTRY(kproc_setenv)      /*< _syscall6(kerrno_t,kproc_setenv,int,self,char const *,name,__size_t,namemax,char const *,value,__size_t,valuemax,int,override); */
  ENTRY(kproc_delenv)      /*< _syscall3(kerrno_t,kproc_delenv,int,self,char const *,name,__size_t,namemax); */
  ENTRY(kproc_enumenv)     /*< _syscall4(kerrno_t,kproc_enumenv,int,self,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
  ENTRY(kproc_getcmd)      /*< _syscall4(kerrno_t,kproc_getcmd,int,self,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
GROUPEND

GROUPBEGIN(sys_mod)
  ENTRY(kmod_open)  /* _syscall4(kerrno_t,kmod_open,char const *,name,size_t,namemax,kmodid_t *,modid,__u32,flags); */
  ENTRY(kmod_fopen) /* _syscall3(kerrno_t,kmod_open,int,fd,kmodid_t *,modid,__u32,flags); */
  ENTRY(kmod_close) /* _syscall1(kerrno_t,kmod_close,kmodid_t,modid); */
  ENTRY(kmod_sym)   /* _syscall3(void *,kmod_sym,kmodid_t,modid,char const *,name,size_t,namemax); */
  ENTRY(kmod_info)  /* _syscall5(kerrno_t,kmod_info,kmodid_t,modid,struct kmodinfo *,buf,size_t,bufsize,size_t *,reqsize,__u32,flags); */
GROUPEND
