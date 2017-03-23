
struct __libc_debug {
    char const *__file;
    int         __line;
    char const *__func;
};





































#line 105





#line 134







#line 142





#line 151






#line 30


#line 33


#line 36


#line 39



#line 44









#line 54




#line 59


#line 62






 




























 

















 
























#line 27


#line 30




#line 35










#line 46





#line 52









#line 67






















































































#line 154




 





 










#line 176


#line 180













#line 193












#line 205


















#line 224









#line 236














#line 252









#line 73




#line 79






 


























 







































#line 90



#line 94










#line 106



#line 121



#line 126












#line 140


#line 144



#line 160



#line 165

 







#line 176


#line 180


#line 184


#line 188


#line 192


#line 196


#line 200


#line 208


#line 212


#line 216


















#line 240


#line 244


#line 248


#line 252


#line 256


#line 260


#line 268


#line 272


#line 276


#line 280


#line 284


#line 288


#line 292


 










#line 311



#line 316


#line 320


#line 324


#line 328





#line 336









#line 346






#line 360




#line 366


#line 370



#line 374


#line 377



#line 383




#line 391







#line 403






#line 410


 



































 









































#line 491






#line 500





 
































 






























 



























typedef __builtin_va_list va_list;





#line 63



















 


























 

























 




























#line 32





 






































 

























#line 28


#line 31


#line 34










 




























 
























 























 


#line 35



























typedef signed char   __s8;

#line 38


typedef signed short  __s16;

#line 43


typedef signed int  __s32;

#line 48


typedef signed long long  __s64;

#line 53


typedef unsigned char  __u8;

#line 58


typedef unsigned short __u16;

#line 63


typedef unsigned int __u32;

#line 68


typedef unsigned long long __u64;

#line 73


















#line 97

typedef __u16  __le16;
typedef __u16  __be16;
typedef __u32  __le32;
typedef __u32  __be32;
typedef __u64  __le64;
typedef __u64  __be64;



#line 114




#line 121



typedef _Bool              __bool_t;

#line 129


typedef char               __char_t;
typedef signed char        __schar_t;
typedef unsigned char      __uchar_t;
typedef short              __short_t;
typedef unsigned short     __ushort_t;
typedef int                __int_t;
typedef unsigned int       __uint_t;
typedef long               __long_t;
typedef unsigned long      __ulong_t;
typedef long long          __llong_t;
typedef unsigned long long __ullong_t;


#line 145

typedef __u16           __char16_t;


#line 150

typedef __u32           __char32_t;



#line 156

typedef __u16          __wchar_t;

#line 160



typedef signed char __int_least8_t;

#line 166


typedef unsigned char __uint_least8_t;

#line 171


typedef signed short __int_least16_t;

#line 176


typedef unsigned short __uint_least16_t;

#line 181


typedef signed int __int_least32_t;

#line 186


typedef unsigned int __uint_least32_t;

#line 191


typedef signed long long __int_least64_t;

#line 196


typedef unsigned long long __uint_least64_t;

#line 201


typedef signed char __int_fast8_t;

#line 206


typedef unsigned char __uint_fast8_t;

#line 211


typedef signed int __int_fast16_t;

#line 216


typedef unsigned int __uint_fast16_t;

#line 221


typedef signed int __int_fast32_t;

#line 226


typedef unsigned int __uint_fast32_t;

#line 231


typedef signed long long __int_fast64_t;

#line 236


typedef unsigned long long __uint_fast64_t;

#line 241


typedef signed int          __intptr_t;

#line 246


typedef unsigned int         __uintptr_t;

#line 251


typedef unsigned int __size_t;

#line 256


typedef signed int __ptrdiff_t;

#line 261


typedef signed long long __intmax_t;

#line 266


typedef unsigned long long __uintmax_t;

#line 271


typedef long double    __max_align_t;
typedef __ptrdiff_t    __ssize_t;
typedef __u8           __byte_t;

typedef __u32          __time32_t;
typedef __u64          __time64_t;

#line 282

typedef __s32          __pid_t;   
typedef unsigned int   __mode_t;
typedef __u32          __dev_t;
typedef __u32          __nlink_t;
typedef __u32          __uid_t;
typedef __u32          __gid_t;
typedef __u32          __ino_t;
typedef int            __openmode_t;
typedef __s64          __loff_t;
typedef __s32          __off32_t;
typedef __s64          __off64_t;
typedef __u32          __pos32_t;
typedef __u64          __pos64_t;
typedef __s32          __blksize32_t;
typedef __s64          __blksize64_t;
typedef __s32          __blkcnt32_t;
typedef __s64          __blkcnt64_t;
typedef __u32          __fsblksize32_t;
typedef __u64          __fsblksize64_t;
typedef __u32          __fsblkcnt32_t;
typedef __u64          __fsblkcnt64_t;
typedef __u32          __fsfilcnt32_t;
typedef __u32          __fsfilcnt64_t;
typedef __u32          __useconds_t;
typedef __s32          __suseconds_t;
typedef unsigned int   __socklen_t;  
typedef __u16          __sa_family_t;



typedef __u32 __clock_t;
typedef __u32 __clockid_t;
typedef __u32 __id_t;
typedef __u32 __timer_t;


#line 320

typedef int         __pthread_t;  



typedef struct __key                 __key_t;
typedef struct __pthread_attr        __pthread_attr_t;
typedef struct __pthread_cond        __pthread_cond_t;
typedef struct __pthread_condattr    __pthread_condattr_t;
typedef __size_t                     __pthread_key_t;
typedef struct __pthread_mutex       __pthread_mutex_t;
typedef struct __pthread_mutexattr   __pthread_mutexattr_t;
typedef struct __pthread_once        __pthread_once_t;
typedef struct __pthread_rwlock      __pthread_rwlock_t;
typedef struct __pthread_rwlockattr  __pthread_rwlockattr_t;
typedef struct __pthread_spinlock    __pthread_spinlock_t;
typedef struct __pthread_barrierattr __pthread_barrierattr_t;
typedef struct __pthread_barrier     __pthread_barrier_t;



typedef __off64_t       __off_t;
typedef __pos64_t       __pos_t;
typedef __blksize64_t   __blksize_t;
typedef __blkcnt64_t    __blkcnt_t;
typedef __fsblksize64_t __fsblksize_t;
typedef __fsblkcnt64_t  __fsblkcnt_t;
typedef __fsfilcnt64_t  __fsfilcnt_t;

#line 356



#line 360

typedef __time32_t  __time_t;


typedef __s16    __ktaskprio_t;
typedef __u16    __kfdtype_t;

typedef __size_t __ktid_t;
typedef __size_t __ktls_t;
typedef int      __ksandbarrier_t;
typedef __size_t __kmodid_t;
typedef __u32    __ktaskopflag_t;






#line 379









 


























 
























 
























 


























 
typedef int (*pformatprinter) (char const *__restrict __data,
#line 34
                                   __size_t __maxchars,
#line 34
                                   void *__closure)
#line 36
;
typedef int (*pformatscanner) (int *__restrict __ch, void *__closure);
typedef int (*pformatreturn) (int __ch, void *__closure);







































extern __attribute__((__nonnull__ (1,3)))  int
format_printf (pformatprinter __printer, void *__closure,
#line 79
                   char const *__restrict __format, ...)
#line 80
;
extern __attribute__((__nonnull__ (1,3)))  int
format_vprintf (pformatprinter __printer, void *__closure,
#line 82
                    char const *__restrict __format, va_list __args)
#line 83
;
























extern __attribute__((__nonnull__ (1,4)))  int
format_scanf (pformatscanner __scanner, pformatreturn __returnch,
#line 109
                  void *__closure, char const *__restrict __format, ...)
#line 110
;
extern __attribute__((__nonnull__ (1,4)))  int
format_vscanf (pformatscanner __scanner, pformatreturn __returnch,
#line 112
                   void *__closure, char const *__restrict __format, va_list __args)
#line 113
;



































struct tm;
extern __attribute__((__nonnull__ (1,4)))  int
format_strftime (pformatprinter __printer, void *__closure,
#line 151
                     char const *__restrict __format, struct tm const *__tm)
#line 152
;






















extern __attribute__((__nonnull__ (1,3))) int
format_quote (pformatprinter __printer, void *__closure,
#line 176
                  char const *__restrict __text, __size_t __maxtext,
#line 176
                  __u32 __flags)
#line 178
;



























extern __attribute__((__nonnull__ (1,3))) int
format_dequote (pformatprinter __printer, void *__closure,
#line 207
                    char const *__restrict __text, __size_t __maxtext)
#line 208
;












extern __attribute__((__nonnull__ (1,3))) int
format_hexdump (pformatprinter __printer, void *__closure,
#line 222
                    void const *__restrict __data, __size_t __size,
#line 222
                    __size_t __linesize, __u32 __flags)
#line 224
;








struct stringprinter {
 char *sp_bufpos;  
 char *sp_buffer;  
 char *sp_bufend;  
};



















extern              __attribute__((__nonnull__ (1))) int   stringprinter_init (struct stringprinter *__restrict __self, __size_t __hint);
extern __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1))) char *stringprinter_pack (struct stringprinter *__restrict __self, __size_t *__length);
extern              __attribute__((__nonnull__ (1))) void  stringprinter_quit (struct stringprinter *__restrict __self);
extern int stringprinter_print (char const *__restrict __data, __size_t __maxchars, void *__closure);




#line 286










 


























 


























typedef int kerrno_t;




































#line 69





























 


























 
























 
























 


























 












































































































































































#line 71


 
 










 











































#line 137





#line 150













#line 174


#line 192







 








































#line 56

static __inline__ kerrno_t k_syslog(int __level, char const * __s, __size_t __maxlen) { kerrno_t __r; (void)(0); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (0), "c" (__level), "d" (__s), "b" (__maxlen) : "memory"); return __r; };
static __attribute__((__unused__)) int __ksyslog_callback(char const *s, __size_t maxlen, void *level) { return k_syslog((int)(__uintptr_t)level,s,maxlen); }








#line 70






 






























 


























#line 45








 
























 
























 







































typedef __off_t off_t;




typedef __pos64_t fpos_t;



#line 63

typedef struct __filestruct FILE;
struct __filestruct {
 __u8  __f_flags;    
 __u8  __f_kind;     
 __u16 __f_padding;  
 union{
  int  __f_fd;  
 };
};








extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;










extern __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1,2))) FILE *
fopen (char const *__restrict __filename,
#line 95
           char const *__restrict __mode)
#line 96
;
extern __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1,2))) FILE *
freopen (char const *__restrict __filename,
#line 98
             char const *__restrict __mode,
#line 98
             FILE *__restrict __fp)
#line 100
;

extern __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (2))) FILE *
fdopen (int __fd, char const *__restrict __mode);

extern __attribute__((__nonnull__ (1))) int fclose (FILE *__restrict __fp);

extern __attribute__((__nonnull__ (1))) int fflush (FILE *__restrict __fp);

extern __attribute__((__nonnull__ (1,2))) int fgetpos(FILE *__restrict __fp, fpos_t *__restrict __pos);
extern __attribute__((__nonnull__ (1,2))) int fsetpos(FILE *__restrict __fp, fpos_t const *__restrict __pos);
extern __attribute__((__nonnull__ (1))) int fseek(FILE *__restrict __fp, long __off, int __whence);
extern __attribute__((__nonnull__ (1))) int fseeko(FILE *__restrict __fp, off_t __off, int __whence);
extern __attribute__((__nonnull__ (1))) long ftell (FILE *__restrict __fp);
extern __attribute__((__nonnull__ (1))) off_t ftello (FILE *__restrict __fp);
extern __attribute__((__nonnull__ (1))) void rewind (FILE *__restrict __fp);
extern __attribute__((__nonnull__ (1))) int fpurge (FILE *__restrict __fp);
extern int getchar(void);

extern __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1,4))) __size_t fread (void *__restrict __buf, __size_t __size,
#line 119
                                                      __size_t __count, FILE *__restrict __fp)
#line 120
;
extern __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1,4))) __size_t fwrite (const void *__restrict __buf, __size_t __size,
#line 121
                                                       __size_t __count, FILE *__restrict __fp)
#line 122
;
extern __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) int fgetc (FILE *__restrict __fp);
extern __attribute__((__nonnull__ (2))) int fputc (int c, FILE *__restrict __fp);
extern __attribute__((__nonnull__ (1,3))) char *fgets (char *__restrict __buf, int bufsize, FILE *__restrict __fp);
extern __attribute__((__nonnull__ (1,2))) int fputs (char const *__restrict __s, FILE *__restrict __fp);

extern __attribute__((__nonnull__ (1))) int getc (FILE *__restrict __fp);
extern __attribute__((__nonnull__ (2))) int putc (int c, FILE *__restrict __fp);



extern __attribute__((__nonnull__ (1))) int getw (FILE *__restrict __fp);
extern __attribute__((__nonnull__ (2))) int putw (int w, FILE *__restrict __fp);

extern __attribute__((__nonnull__ (1,2)))  int fprintf (FILE *__restrict __fp, char const *__restrict __fmt, ...);
extern __attribute__((__nonnull__ (1,2)))  int vfprintf (FILE *__restrict __fp, char const *__restrict __fmt, va_list __args);



#line 143

extern __attribute__((__nonnull__ (1))) int fseek64 (FILE *__restrict __fp, __s64 __off, int __whence);
extern __attribute__((__nonnull__ (1))) __u64 ftell64 (FILE *__restrict __fp);



extern __attribute__((__nonnull__ (1))) int _fseek64 (FILE *__restrict __fp, __s64 __off, int __whence);
extern __attribute__((__nonnull__ (1))) __u64 _ftell64 (FILE *__restrict __fp);

extern __attribute__((__nonnull__ (1,2)))  int fscanf (FILE *__restrict __fp, char const *__restrict __fmt, ...);
extern __attribute__((__nonnull__ (1,2)))  int vfscanf (FILE *__restrict __fp, char const *__restrict __fmt, va_list __args);


extern __attribute__((__nonnull__ (1))) int fileno (FILE *__restrict __fp);
















extern int putchar (int c);
extern __attribute__((__nonnull__ (1))) int puts (char const *__restrict __s);
extern __attribute__((__nonnull__ (1)))  int printf (char const *__restrict __format, ...);
extern __attribute__((__nonnull__ (1)))  int vprintf (char const *__restrict __format, va_list __args);

extern __attribute__((__nonnull__ (2)))  __size_t _sprintf (char *__restrict __buf, char const *__restrict __format, ...);
extern __attribute__((__nonnull__ (2)))  __size_t _vsprintf (char *__restrict __buf, char const *__restrict __format, va_list __args);
extern __attribute__((__nonnull__ (3)))  __size_t _snprintf (char *__restrict __buf, __size_t bufsize, char const *__restrict __format, ...);
extern __attribute__((__nonnull__ (3)))  __size_t _vsnprintf (char *__restrict __buf, __size_t bufsize, char const *__restrict __format, va_list __args);



#line 190

 


extern __attribute__((__nonnull__ (2)))  __size_t sprintf (char *__restrict __buf, char const *__restrict __format, ...) __asm__("_sprintf");
extern __attribute__((__nonnull__ (2)))  __size_t vsprintf (char *__restrict __buf, char const *__restrict __format, va_list __args) __asm__("_vsprintf");
extern __attribute__((__nonnull__ (3)))  __size_t snprintf (char *__restrict __buf, __size_t bufsize, char const *__restrict __format, ...) __asm__("_snprintf");
extern __attribute__((__nonnull__ (3)))  __size_t vsnprintf (char *__restrict __buf, __size_t bufsize, char const *__restrict __format, va_list __args) __asm__("_vsnprintf");

#line 203




extern __attribute__((__nonnull__ (1,2)))  int sscanf (char const *__restrict __data, char const *__restrict __format, ...);
extern __attribute__((__nonnull__ (1,2)))  int vsscanf (char const *__restrict __data, char const *__restrict __format, va_list __args);
extern __attribute__((__nonnull__ (1,3)))  int _snscanf (char const *__restrict __data, __size_t __maxdata, char const *__restrict __format, ...);
extern __attribute__((__nonnull__ (1,3)))  int _vsnscanf (char const *__restrict __data, __size_t __maxdata, char const *__restrict __format, va_list __args);



extern __attribute__((__nonnull__ (1,3)))  int snscanf (char const *__restrict __data, __size_t __maxdata, char const *__restrict __format, ...) __asm__("_snscanf");
extern __attribute__((__nonnull__ (1,3)))  int vsnscanf (char const *__restrict __data, __size_t __maxdata, char const *__restrict __format, va_list __args) __asm__("_vsnscanf");

#line 219









extern __attribute__((__nonnull__ (1))) int remove (char const *__pathname);
extern __attribute__((__nonnull__ (2))) int removeat (int __dirfd, char const *__pathname);




extern __attribute__((__cold__)) __attribute__((__nonnull__ (1))) void perror (char const *__s);



extern __attribute__((__nonnull__ (2)))  int dprintf (int __fd, char const *__restrict __format, ...);
extern __attribute__((__nonnull__ (2)))  int vdprintf (int __fd, char const *__restrict __format, va_list __args);










 


























static __attribute__((__unused__)) void outf(char const *fmt, ...) {
 va_list args;
 __builtin_va_start(args,fmt);
 format_vprintf(&__ksyslog_callback,(void *)(__uintptr_t)(4),fmt,args);
 __builtin_va_end(args);
 __builtin_va_start(args,fmt);
 vprintf(fmt,args);
 __builtin_va_end(args);
}




 
























 


























 
























 






























#line 41




extern __attribute__((__noinline__)) __attribute__((__noclone__)) __attribute__((__noreturn__)) __attribute__((__cold__))

       

#line 52

void __assertion_failedf (char const *__file, int __line, char const *__func, char const *__expr,
#line 53
                              unsigned int __skip, char const *__fmt, ...)
#line 54
;
extern __attribute__((__noinline__)) __attribute__((__noclone__)) __attribute__((__noreturn__)) __attribute__((__cold__))

       

#line 62

void __assertion_failedxf (struct __libc_debug const *__dbg, char const *__expr,
#line 63
                               unsigned int __skip, char const *__fmt, ...)
#line 64
;
extern __attribute__((__noinline__)) __attribute__((__noclone__)) __attribute__((__noreturn__)) __attribute__((__cold__))
void __libc_unreachable_d (char const *__file, int __line, char const *__func);












#line 81









#line 92


#line 111










#line 35















 






























 

























 


























 

























#line 49



#line 84










 

























 
























 

























#line 42


#line 46


#line 50


#line 54


#line 58


#line 62


#line 66


#line 70



 













typedef __size_t size_t;



#line 92



#line 98




extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__assume_aligned__(8))) void *memdup (void const *__p, __size_t __bytes) __asm__("_memdup");
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1)))  __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *strdupf (char const *__restrict __format, ...) __asm__("_strdupf");
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1)))  __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *vstrdupf (char const *__restrict __format, va_list __args) __asm__("_vstrdupf");

#line 109




 














extern __attribute__((__warn_unused_result__)) __attribute__((__returns_nonnull__)) __attribute__((__malloc__))  char *__attribute__((__cdecl__)) _strdupaf(char const *__restrict __fmt, ...);
extern __attribute__((__warn_unused_result__)) __attribute__((__returns_nonnull__)) __attribute__((__malloc__))  char *__attribute__((__cdecl__)) _vstrdupaf(char const *__restrict __fmt, va_list args);

#line 131

 






























#line 166


#line 171


#line 175


 

#line 181










extern __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1,2))) void *
memcpy (void *__restrict __dst,
#line 192
            void const *__restrict __src,
#line 192
            __size_t __bytes)
#line 194
;



extern __attribute__((__nonnull__ (1,2))) void *
memccpy (void *__restrict __dst,
#line 199
             void const *__restrict __src,
#line 199
             int __c, __size_t __bytes)
#line 201
;



extern __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1))) void *
memset (void *__restrict __dst,
#line 206
            int __byte, __size_t __bytes)
#line 207
;


extern __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1,2))) void *memmove (void *__dst, void const *__src, __size_t __bytes);


extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int memcmp (void const *__a, void const *__b, __size_t __bytes);


extern __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1,2))) char *strcpy (char *__dst, char const *__src);
extern __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1,2))) char *strncpy (char *__dst, char const *__src, __size_t __maxchars);


extern __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1,2))) char *strcat (char *__dst, char const *__src);
extern __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1,2))) char *strncat (char *__dst, char const *__src, __size_t __maxchars);









#line 312



extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) void *memchr (void const *__restrict __p, int __needle, size_t __bytes);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) void *memrchr (void const *__restrict __p, int __needle, size_t __bytes);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) void *memmem (void const *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) void *_memrmem (void const *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen);



extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strchr (char const *__restrict __haystack, int __needle);



extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strrchr (char const *__restrict __haystack, int __needle);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strnchr (char const *__restrict __haystack, __size_t __max_haychars, int __needle);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strnrchr (char const *__restrict __haystack, __size_t __max_haychars, int __needle);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *strstr (char const *__haystack, char const *__needle);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *_strrstr (char const *__haystack, char const *__needle);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *_strnstr (char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *_strnrstr (char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *strpbrk (char const *__haystack, char const *__needle_list);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *_strrpbrk (char const *__haystack, char const *__needle_list);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *_strnpbrk (char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *_strnrpbrk (char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1))) char *_strend (char const *__restrict __s);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1))) char *_strnend (char const *__restrict __s, __size_t __maxchars);


extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) void *memrmem (void const *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen) __asm__("_memrmem");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strnchr (char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asm__("_strnchr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strnrchr (char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asm__("_strnrchr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *strrstr (char const *__haystack, char const *__needle) __asm__("_strrstr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *strnstr (char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asm__("_strnstr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *strnrstr (char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asm__("_strnrstr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *strrpbrk (char const *__haystack, char const *__needle_list) __asm__("_strrpbrk");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *strnpbrk (char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asm__("_strnpbrk");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *strnrpbrk (char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asm__("_strnrpbrk");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1))) char *strend (char const *__restrict __s) __asm__("_strend");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__returns_nonnull__)) __attribute__((__nonnull__ (1))) char *strnend (char const *__restrict __s, __size_t __maxchars) __asm__("_strnend");

#line 370




extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) __size_t strlen (char const *__restrict __s);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) __size_t strnlen (char const *__restrict __s, __size_t __maxchars);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int strcmp (char const *__a, char const *__b);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int strncmp (char const *__a, char const *__b, __size_t __maxchars);


 



extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int _strwcmp (char const *__restrict __str, char const *__restrict __wildpattern);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int _striwcmp (char const *__restrict __str, char const *__restrict __wildpattern);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) int _strnwcmp (char const *__restrict __str, __size_t __maxstr, char const *__restrict __wildpattern, __size_t __maxpattern);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) int _strinwcmp (char const *__restrict __str, __size_t __maxstr, char const *__restrict __wildpattern, __size_t __maxpattern);


extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int strwcmp (char const *__restrict __str, char const *__restrict __wildpattern) __asm__("_strwcmp");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int striwcmp (char const *__restrict __str, char const *__restrict __wildpattern) __asm__("_striwcmp");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) int strnwcmp (char const *__restrict __str, __size_t __maxstr, char const *__restrict __wildpattern, __size_t __maxpattern) __asm__("_strnwcmp");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) int strinwcmp (char const *__restrict __str, __size_t __maxstr, char const *__restrict __wildpattern, __size_t __maxpattern) __asm__("_strinwcmp");

#line 400




 




extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) __size_t strspn (char const *__str, char const *__spanset);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) __size_t _strnspn (char const *__str, __size_t __maxstr, char const *__spanset, __size_t __maxspanset);

 




extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) __size_t strcspn (char const *__str, char const *__spanset);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) __size_t _strncspn (char const *__str, __size_t __maxstr, char const *__spanset, __size_t __maxspanset);



extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) __size_t strnspn (char const *__str, __size_t __maxstr, char const *__spanset, __size_t __maxspanset) __asm__("_strnspn");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) __size_t strncspn (char const *__str, __size_t __maxstr, char const *__spanset, __size_t __maxspanset) __asm__("_strncspn");

#line 427




 
 

extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strset (char *__restrict __dst, int __chr);
extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strnset (char *__restrict __dst, int __chr, __size_t __maxlen);



extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strset (char *__restrict __dst, int __chr) __asm__("_strset");
extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strnset (char *__restrict __dst, int __chr, __size_t __maxlen) __asm__("_strnset");

#line 444



#line 459




 




 




extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) void *_memrev (void *__p, __size_t __bytes);

#line 481









 



extern __attribute__((__pure__)) char *_strlwr (char *__str);
extern __attribute__((__pure__)) char *_strupr (char *__str);
extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strnlwr (char *__str, __size_t __maxlen);
extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strnupr (char *__str, __size_t __maxlen);

 

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int _strcmpi (char const *__a, char const *__b) __asm__("_stricmp");

#line 504




extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) void *memrev (void *__p, __size_t __bytes) __asm__("_memrev");
extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strlwr (char *__str) __asm__("_strlwr");
extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strupr (char *__str) __asm__("_strupr");
extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strnlwr (char *__str, __size_t __maxlen) __asm__("_strnlwr");
extern __attribute__((__returns_nonnull__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strnupr (char *__str, __size_t __maxlen) __asm__("_strnupr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int strcmpi (char const *__a, char const *__b) __asm__("_stricmp");

#line 521









 
 





extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int _stricmp (char const *__a, char const *__b);



extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int _strincmp (char const *__a, char const *__b, __size_t __maxchars);



extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int _memicmp (void const *__a, void const *__b, __size_t __bytes);




extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int stricmp (char const *__a, char const *__b) __asm__("_stricmp");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int strincmp (char const *__a, char const *__b, __size_t __maxchars) __asm__("_strincmp");

#line 555




extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) int memicmp (void const *__a, void const *__b, __size_t __bytes) __asm__("_memicmp");

#line 562





#line 633

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) void *_memichr (void const *__restrict __p, int __needle, size_t __bytes);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) void *_memirchr (void const *__restrict __p, int __needle, size_t __bytes);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strichr (char const *__restrict __haystack, int __needle);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strirchr (char const *__restrict __haystack, int __needle);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strinchr (char const *__restrict __haystack, __size_t __max_haychars, int __needle);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *_strinrchr (char const *__restrict __haystack, __size_t __max_haychars, int __needle);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *_stristr (char const *__haystack, char const *__needle);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *_strirstr (char const *__haystack, char const *__needle);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *_strinstr (char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *_strinrstr (char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars);

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *_stripbrk (char const *__haystack, char const *__needle_list);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *_strirpbrk (char const *__haystack, char const *__needle_list);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *_strinpbrk (char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist);
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *_strinrpbrk (char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist);



extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) void *memichr (void const *__restrict __p, int __needle, size_t __bytes) __asm__("_memichr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) void *memirchr (void const *__restrict __p, int __needle, size_t __bytes) __asm__("_memirchr");

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strichr (char const *__restrict __haystack, int __needle) __asm__("_strichr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strirchr (char const *__restrict __haystack, int __needle) __asm__("_strirchr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strinchr (char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asm__("_strinchr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1))) char *strinrchr (char const *__restrict __haystack, __size_t __max_haychars, int __needle) __asm__("_strinrchr");

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *stristr (char const *__haystack, char const *__needle) __asm__("_stristr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *strirstr (char const *__haystack, char const *__needle) __asm__("_strirstr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *strinstr (char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asm__("_strinstr");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *strinrstr (char const *__haystack, __size_t __max_haychars, char const *__needle, __size_t __max_needlechars) __asm__("_strinrstr");

extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *stripbrk (char const *__haystack, char const *__needle_list) __asm__("_stripbrk");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,2))) char *strirpbrk (char const *__haystack, char const *__needle_list) __asm__("_strirpbrk");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *strinpbrk (char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asm__("_strinpbrk");
extern __attribute__((__warn_unused_result__)) __attribute__((__pure__)) __attribute__((__nonnull__ (1,3))) char *strinrpbrk (char const *__haystack, __size_t __max_haychars, char const *__needle_list, __size_t __max_needlelist) __asm__("_strinrpbrk");

#line 686







extern __attribute__((__nonnull__ (2,3))) char *strtok_r (char *__restrict __str, const char *__restrict __delim, char **__restrict __saveptr);
extern __attribute__((__nonnull__ (2,4))) char *_strntok_r (char *__restrict __str, const char *__restrict __delim, __size_t __delim_max, char **__restrict __saveptr);

extern __attribute__((__nonnull__ (2,3))) char *_stritok_r (char *__restrict __str, const char *__restrict __delim, char **__restrict __saveptr);
extern __attribute__((__nonnull__ (2,4))) char *_strintok_r (char *__restrict __str, const char *__restrict __delim, __size_t __delim_max, char **__restrict __saveptr);



extern __attribute__((__nonnull__ (2))) char *strtok (char *__restrict __str, const char *__restrict __delim);





















extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *strdup (char const *__restrict __s);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *strndup (char const *__restrict __s, __size_t __maxchars);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__assume_aligned__(8))) void *_memdup (void const *__restrict __p, __size_t __bytes);





extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1)))  __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_strdupf (char const *__restrict __format, ...);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1)))  __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_vstrdupf (char const *__restrict __format, va_list __args);




extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_strdup_d (char const *__restrict __s ,char const *__file, int __line, char const *__func);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_strndup_d (char const *__restrict __s, __size_t __maxchars ,char const *__file, int __line, char const *__func);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__assume_aligned__(8))) void *_memdup_d (void const *__restrict __p, __size_t __bytes ,char const *__file, int __line, char const *__func);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1)))  __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_strdupf_d (char const *__file, int __line, char const *__func, char const *__restrict __format, ...);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1)))  __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_vstrdupf_d (char const *__restrict __format, va_list __args ,char const *__file, int __line, char const *__func);


extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_strdup_x (char const *__restrict __s ,struct __libc_debug const *__dbg);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_strndup_x (char const *__restrict __s, __size_t __maxchars ,struct __libc_debug const *__dbg);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__assume_aligned__(8))) void *_memdup_x (void const *__restrict __p, __size_t __bytes ,struct __libc_debug const *__dbg);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1)))  __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_strdupf_x (struct __libc_debug const *__dbg, char const *__restrict __format, ...);
extern  __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1)))  __attribute__((__malloc__)) __attribute__((__assume_aligned__(8))) char *_vstrdupf_x (char const *__restrict __format, va_list __args ,struct __libc_debug const *__dbg);















#line 769











#line 785




#line 797


#line 802


#line 806


#line 814







#line 838



extern char *strerror (int __eno);
extern char *strerror_r (int __eno, char *__restrict __buf, __size_t __buflen);





#line 851






#line 860


























#line 1088



 

#line 1095


#line 1099


#line 1103


#line 1107


#line 1111


#line 1115


#line 1119


#line 1123









 






























 
























 


























typedef int errno_t;





 


























 

























 
























 
























#line 57





#line 66











#line 79




#line 84



#line 101



#line 118





 





























 



#line 36



#line 42












#line 128









 

#line 141


#line 145


#line 149


#line 153



#line 157








 



#line 49







#line 61






static __attribute__((__always_inline__)) __inline__ __attribute__((__cold__)) void __set_errno (errno_t __eno) { __extension__({ __asm__("movl %1, %%" "gs" ":(%0)" : : "r" (64), "r" (__eno)); (void)0; }); }



#line 73


 









 
































































 


 





























#line 35















 


























 

























 

























 


























 
























 

























 


























 
























 
























 


























 
























 


























#line 33











#line 47



#line 51


 











































































































































#line 199



#line 208




#line 213









 


























 
























 


























 



























 






























struct timespec {
 __time_t tv_sec;   
 long     tv_nsec;  
};




#line 48


#line 54

















 
























 
























 

































struct __attribute__((__packed__)) kexinfo {
 __u32 ex_no;                      
 __u32 ex_info;                    
 void *ex_ptr[4];  
};















struct __attribute__((__packed__)) kexstate { __u32 edi,esi,ebp,esp,ebx,edx,ecx,eax,eip,eflags; };


#line 70



typedef void (*__attribute__((__noreturn__)) kexhandler_t)();
struct kexrecord {
  
  struct kexrecord *eh_prev;     
 kexhandler_t             eh_handler;  
};





















struct __attribute__((__packed__)) kuthread {
  
  
  
  struct kuthread   *u_self;          
  struct kuthread   *u_parent;        
  struct kexrecord  *u_exh;           
  void              *u_stackbegin;    
  void              *u_stackend;      
  void              *u_padding1[11];  
  
union __attribute__((__packed__)) { struct __attribute__((__packed__)) {
 int                       u_errno;         
 __pid_t                   u_pid;           
 __ktid_t                  u_tid;           
 __size_t                  u_parid;         
 struct timespec           u_start;         
};  void            *u_padding2[16];  };
  
 char                      u_name[47];  

 char                      u_zero;          
 struct kexinfo            u_exinfo;        
 struct kexstate           u_exstate;       
  
};



#line 132






struct ktlsinfo {
 __ptrdiff_t ti_offset;  
 __size_t    ti_size;    
};

























static __inline__ kerrno_t kproc_tlsfree(__ptrdiff_t __tls_offset) { kerrno_t __r; (void)(71); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (71), "c" (__tls_offset) : "memory"); return __r; };
static __inline__ kerrno_t
kproc_tlsalloc(void const *__template,
               __size_t __template_size,
               __ptrdiff_t *__restrict __tls_offset) {
 kerrno_t __error;
 __asm__("int $" "0x69" "\n"
         : "=a" (__error), "=c" (*__tls_offset)
         : "a" (70)
         , "c" (__template)
         , "d" (__template_size));
 return __error;
}








static __inline__ kerrno_t
kproc_tlsenum(struct ktlsinfo *__restrict __infov,
              __size_t __infoc, __size_t *__restrict __reqinfoc) {
 kerrno_t __error;
 __size_t __val_reqinfoc;
 __asm__("int $" "0x69" "\n"
         : "=a" (__error), "=c" (__val_reqinfoc)
         : "a" (72), "c" (__infov), "d" (__infoc));
 if (__reqinfoc) *__reqinfoc = __val_reqinfoc;
 return __error;
}










 

























 





























#line 35

















 






























 
























 
























 





























#line 41









#line 52





typedef __ssize_t ssize_t;





typedef __ptrdiff_t ptrdiff_t;




typedef __max_align_t max_align_t;





typedef __WCHAR_TYPE__ wchar_t;













 


























 
























 
























 


























 





























#line 50



































































 





















































 



#line 139



#line 158




 













#line 184


#line 194


#line 200


 

#line 208


#line 210


 



#line 223


#line 225




























#line 258


#line 261


#line 264




 

#line 272




 

#line 280


#line 284







































#line 324

 












 
















#line 356



#line 366



#line 371











#line 428











typedef __u32 kexno_t;











extern __attribute__((__noreturn__)) void __kos_unhandled_exception(void);



#line 464





static __inline__ __attribute__((__noreturn__)) void kexcept_rethrow (void) {
 struct kexrecord *handler;
 ((__builtin_expect(!!(!(__extension__({ register __u32 __tls_res; __asm__("movl %%" "gs" ":(%1), %0" : "=r" (__tls_res) : "r" ((4*32+47+1)+0));  __tls_res; }) != 0)),0))?__assertion_failedxf(__extension__({ static struct __libc_debug const __dbg = {"E:\\c\\kos\\kos\\include\\kos\\exception.h",471,__func__};              &__dbg; }), "kexcept_code() != KEXCEPTION_NONE",0,"Not handling any exceptions right now"):(void)0)
#line 472
;
  
 handler = (struct kexrecord *)__extension__({ register __u32 __tls_res; __asm__("movl %%" "gs" ":(%1), %0" : "=r" (__tls_res) : "r" (8));  __tls_res; });
 if (__builtin_expect(!!(!handler),0)) __kos_unhandled_exception();
  
 __asm__ __volatile__("mov %0, %%esp\n"
                  "pop %%" "gs" ":8" "\n"
                  "ret\n" : : "r" (handler));
 __libc_unreachable_d("E:\\c\\kos\\kos\\include\\kos\\exception.h",480,__func__);
}
static __inline__ __attribute__((__noreturn__)) void __kexcept_throw (struct kexinfo const * __exc) {
 memcpy((&((struct kuthread *)__extension__({ register __byte_t *__tls_res; __asm__("movl %%" "gs" ":0, %0" : "=r" (__tls_res));  __tls_res; }))->u_exinfo),__exc,sizeof(struct kexinfo));
 kexcept_rethrow();
}




#line 503

static __attribute__((__always_inline__)) __inline__ __attribute__((__noreturn__)) void kexcept_continue (void) {
 ((__builtin_expect(!!(!(__extension__({ register __u32 __tls_res; __asm__("movl %%" "gs" ":(%1), %0" : "=r" (__tls_res) : "r" ((4*32+47+1)+0));  __tls_res; }) != 0)),0))?__assertion_failedxf(__extension__({ static struct __libc_debug const __dbg = {"E:\\c\\kos\\kos\\include\\kos\\exception.h",505,__func__};              &__dbg; }), "kexcept_code() != KEXCEPTION_NONE",0,"Not handling any exceptions right now"):(void)0)
#line 506
;
 __asm__ __volatile__("pushl " "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+36" ")"  "\n"
                  "popf"                                      "\n"
                  "mov " "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+28" ")" ",%%eax\n"
                  "mov " "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+24" ")" ",%%ecx\n"
                  "mov " "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+20" ")" ",%%edx\n"
                  "mov " "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+16" ")" ",%%ebx\n"
                  "mov " "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+12" ")" ",%%esp\n"
                  "mov " "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+8" ")" ",%%ebp\n"
                  "mov " "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+4" ")" ",%%esi\n"
                  "mov " "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+0" ")" ",%%edi\n"
                  "jmp *" "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+32" ")" "\n"
                  : : : "memory");
 __libc_unreachable_d("E:\\c\\kos\\kos\\include\\kos\\exception.h",519,__func__);
}

#line 523

static __inline__ void kexcept_throw (struct kexinfo const * __exc) {
 ((__builtin_expect(!!(!(__exc->ex_no != 0)),0))?__assertion_failedxf(__extension__({ static struct __libc_debug const __dbg = {"E:\\c\\kos\\kos\\include\\kos\\exception.h",525,__func__};              &__dbg; }), "__exc->ex_no != KEXCEPTION_NONE",0,"Can't throw exception code ZERO(0)"):(void)0)
#line 526
;
 __extension__({ __asm__ goto("mov %%edi," "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+0" ")" "\n"                         "mov %%esi," "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+4" ")" "\n"                         "mov %%ebp," "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+8" ")" "\n"                         "mov %%esp," "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+12" ")" "\n"                         "mov %%ebx," "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+16" ")" "\n"                         "mov %%edx," "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+20" ")" "\n"                         "mov %%ecx," "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+24" ")" "\n"                         "mov %%eax," "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+28" ")" "\n"                         "movl $%l0," "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+32" ")" "\n"                         "pushf"                                     "\n"                         "pop "    "%%" "gs" ":(" "(4*32+47+1+(8+(4)*4))+36" ")" "\n"                         : : : "memory" : __done);            (void)0; });
 __kexcept_throw(__exc);
__done:;
}
static __inline__ void kexcept_raise (kexno_t __no) {
 struct kexinfo __exc;
 memset(&__exc,0,sizeof(struct kexinfo));
 __exc.ex_no = __no;
 kexcept_throw(&__exc);
}









#line 34






typedef int (*ptbwalker) (void const *__restrict __instruction_pointer,
#line 40
                              void const *__restrict __frame_address,
#line 40
                              __size_t __frame_index, void *__closure)
#line 42
;



typedef int (*ptberrorhandler) (int __error, void const *__arg, void *__closure);


typedef kexno_t        exno_t;
typedef struct kexinfo exception_t;

 
extern __attribute__((__noreturn__)) void exc_throw (exception_t const *__restrict __exception);
extern __attribute__((__noreturn__)) void exc_raise (exno_t __exception_number);
extern void exc_throw_resumeable (exception_t const *__restrict __exception);
extern void exc_raise_resumeable (exno_t __exception_number);
extern __attribute__((__noreturn__)) void exc_rethrow (void);
extern __attribute__((__noreturn__)) void exc_continue (void);










extern int exc_tbwalk (ptbwalker __callback, ptberrorhandler __handle_error, void *__closure);
extern int exc_tbprint (int __tp_only);
extern int exc_tbprintex (int __tp_only, void *__closure);
extern __attribute__((__warn_unused_result__)) __attribute__((__malloc__)) struct tbtrace *exc_tbcapture (void);
extern __attribute__((__warn_unused_result__)) char const *exc_getname (exno_t __exception_number);











#line 85



#line 94



#line 100















 




























 
























 
























 

























 

























 


























 

























#line 100








































#line 41


#line 45


#line 49


#line 53


#line 57


#line 61


#line 65


#line 69


#line 73


#line 77


#line 81


#line 85



 















#line 107



#line 111


#line 114



#line 120




#line 125

extern  void cfree (void *__restrict __mallptr) __asm__("free");









#line 138

extern  __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__alloc_align__(1)))
void *aligned_alloc (__size_t __alignment, __size_t __bytes) __asm__("memalign");




#line 146












extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1))) __attribute__((__assume_aligned__(8))) void *malloc (__size_t __bytes);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1,2))) __attribute__((__assume_aligned__(8))) void *calloc (__size_t __count, __size_t __bytes);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__alloc_align__(1))) void *memalign (__size_t __alignment, __size_t __bytes);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__nonnull__ (1))) __attribute__((__assume_aligned__(8))) void *realloc (void *__restrict __mallptr, __size_t __bytes);
extern  __attribute__((__nonnull__ (1))) void free (void *__restrict __freeptr);
extern  __attribute__((__nonnull__ (1))) int posix_memalign (void **__restrict __memptr, __size_t __alignment, __size_t __bytes);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1))) __attribute__((__assume_aligned__(4096))) void *pvalloc (__size_t __bytes);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1))) __attribute__((__assume_aligned__(4096))) void *valloc (__size_t __bytes);























extern int mallopt (int __parameter_number,
#line 189
                        int __parameter_value)
#line 190
;









extern int malloc_trim (__size_t __pad);
















extern __attribute__((__warn_unused_result__)) __size_t malloc_usable_size (void *__restrict __mallptr);














extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1))) __attribute__((__assume_aligned__(8))) void *_malloc_d (__size_t __bytes ,char const *__file, int __line, char const *__func);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1,2))) __attribute__((__assume_aligned__(8))) void *_calloc_d (__size_t __count, __size_t __bytes ,char const *__file, int __line, char const *__func);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__nonnull__ (1))) __attribute__((__assume_aligned__(8))) void *_realloc_d (void *__restrict __mallptr, __size_t __bytes ,char const *__file, int __line, char const *__func);
extern __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __size_t _malloc_usable_size_d (void *__restrict __mallptr ,char const *__file, int __line, char const *__func);
extern  __attribute__((__nonnull__ (1))) void _free_d (void *__restrict __freeptr ,char const *__file, int __line, char const *__func);
extern  __attribute__((__nonnull__ (1))) int _posix_memalign_d (void **__restrict __memptr, __size_t __alignment, __size_t __bytes ,char const *__file, int __line, char const *__func);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1))) __attribute__((__assume_aligned__(4096))) void *_pvalloc_d (__size_t __bytes ,char const *__file, int __line, char const *__func);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1))) __attribute__((__assume_aligned__(4096))) void *_valloc_d (__size_t __bytes ,char const *__file, int __line, char const *__func);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__alloc_align__(1))) void *_memalign_d (__size_t __alignment, __size_t __bytes ,char const *__file, int __line, char const *__func);


extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1))) __attribute__((__assume_aligned__(8))) void *_malloc_x (__size_t __bytes ,struct __libc_debug const *__dbg);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1,2))) __attribute__((__assume_aligned__(8))) void *_calloc_x (__size_t __count, __size_t __bytes ,struct __libc_debug const *__dbg);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__nonnull__ (1))) __attribute__((__assume_aligned__(8))) void *_realloc_x (void *__restrict __mallptr, __size_t __bytes ,struct __libc_debug const *__dbg);
extern __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1))) __size_t _malloc_usable_size_x (void *__restrict __mallptr ,struct __libc_debug const *__dbg);
extern  __attribute__((__nonnull__ (1))) void _free_x (void *__restrict __freeptr ,struct __libc_debug const *__dbg);
extern  __attribute__((__nonnull__ (1))) int _posix_memalign_x (void **__restrict __memptr, __size_t __alignment, __size_t __bytes ,struct __libc_debug const *__dbg);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1))) __attribute__((__assume_aligned__(4096))) void *_pvalloc_x (__size_t __bytes ,struct __libc_debug const *__dbg);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (1))) __attribute__((__assume_aligned__(4096))) void *_valloc_x (__size_t __bytes ,struct __libc_debug const *__dbg);
extern  __attribute__((__warn_unused_result__)) __attribute__((__malloc__,__alloc_size__ (2))) __attribute__((__alloc_align__(1))) void *_memalign_x (__size_t __alignment, __size_t __bytes ,struct __libc_debug const *__dbg);







struct _mallblock_d;







extern __attribute__((__nonnull__ (1))) void *
__mallblock_getattrib_d (struct _mallblock_d const *__restrict __self,
#line 268
                             int __attrib)
#line 269
;


#line 283









#line 296







extern __attribute__((__nonnull__ (1,2))) int
_mallblock_traceback_d (struct _mallblock_d *__restrict self,
#line 304
                            ptbwalker __callback,
#line 304
                            void *__closure)
#line 306
;























extern __attribute__((__nonnull__ (2))) int
_malloc_enumblocks_d (void *__checkpoint,
#line 331
                          int (*__callback)(struct _mallblock_d *__restrict __block,
#line 331
                                            void *__closure),
#line 331
                          void *__closure)
#line 334
;









extern void _malloc_printleaks_d (void);









extern void _malloc_validate_d (void);








extern void *__malloc_untrack (void *__mallptr);



#line 370














#line 393






#line 412








#line 421




struct mallinfo {
    __size_t arena;     
    __size_t ordblks;   
    __size_t smblks;    
    __size_t hblks;     
    __size_t hblkhd;    
    __size_t usmblks;   
    __size_t fsmblks;   
    __size_t uordblks;  
    __size_t fordblks;  
    __size_t keepcost;  
};


extern struct mallinfo mallinfo (void);
extern void malloc_stats (void);









 

#line 454


#line 458


#line 462


#line 466


#line 470


#line 474


#line 478


#line 482


#line 486


#line 490


#line 494


#line 498








 


























 

























 
























 


























 
























 

























 
























 
























 































typedef __kmodid_t kmodid_t;


 




















































































static __inline__ kerrno_t kmod_open(char const * __name, __size_t __namemax, kmodid_t * __modid, __u32 __flags) { kerrno_t __r; (void)(82); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (82), "c" (__name), "d" (__namemax), "b" (__modid), "S" (__flags) : "memory"); return __r; }
#line 130
;
static __inline__ kerrno_t kmod_fopen(int __fd, kmodid_t * __modid, __u32 __flags) { kerrno_t __r; (void)(83); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (83), "c" (__fd), "d" (__modid), "b" (__flags) : "memory"); return __r; };























static __inline__ kerrno_t kmod_close(kmodid_t __modid) { kerrno_t __r; (void)(84); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (84), "c" (__modid) : "memory"); return __r; };





































static __inline__ void * kmod_sym(kmodid_t __modid, char const * __name, __size_t __namemax) { void * __r; (void)(85); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (85), "c" (__modid), "d" (__name), "b" (__namemax) : "memory"); return __r; }
#line 194
;






typedef __u32 ksymflag_t;





















struct ksymname {
 char const *sn_name;  
 __size_t    sn_size;  
};

struct ksyminfo {
 ksymflag_t   si_flags;  
 kmodid_t     si_modid;  
 void        *si_base;   
 __size_t     si_size;   
 char        *si_name;   
 __size_t     si_nmsz;   
  
 char        *si_file;   
 __size_t     si_flsz;   
 unsigned int si_line;   




 unsigned int si_col;    
};






















static __inline__ kerrno_t kmod_syminfo(int __procfd, kmodid_t __modid, struct ksymname const * __addr_or_name, struct ksyminfo * __buf, __size_t __bufsize, __size_t * __reqsize, __u32 __flags) { kerrno_t __r; (void)(87); __asm__ __volatile__("pushl %8\npushl %7\n" "int $" "0x69" "\n" "add $8, %%esp" : "=a" (__r) : "a" (87), "c" (__procfd), "d" (__modid), "b" (__addr_or_name), "S" (__buf), "D" (__bufsize), "m" (__reqsize), "m" (__flags) : "memory"); return __r; }
#line 269
;











typedef __u32 kmodkind_t;


 











 









struct kmodinfo {
     





    kmodid_t   mi_id;          
    kmodkind_t mi_kind;        
    void      *mi_base;        
     













    void      *mi_begin;       
    void      *mi_end;         
    void      *mi_start;       
    char      *mi_name;        
    __size_t   mi_nmsz;        
     
};


















static __inline__ kerrno_t kmod_info(int __procfd, kmodid_t __modid, struct kmodinfo * __buf, __size_t __bufsize, __size_t * __reqsize, __u32 __flags) { kerrno_t __r; (void)(86); __asm__ __volatile__("pushl %7\n"           "int $" "0x69" "\n" "add $4, %%esp" : "=a" (__r) : "a" (86), "c" (__procfd), "d" (__modid), "b" (__buf), "S" (__bufsize), "D" (__reqsize), "m" (__flags) : "memory"); return __r; }
#line 357
;




 


 



 





 









 


























 
























 

























 
























 


























 


























 
























#line 29
























































 
























 
























 






























 
























 
























 





























typedef __s8   int_fast8_t;
typedef __u8  uint_fast8_t;
typedef __s16  int_fast16_t;
typedef __u16 uint_fast16_t;
typedef __s32  int_fast32_t;
typedef __u32 uint_fast32_t;
typedef __s64  int_fast64_t;
typedef __u64 uint_fast64_t;




typedef __s8   int_least8_t;
typedef __u8  uint_least8_t;
typedef __s16  int_least16_t;
typedef __u16 uint_least16_t;
typedef __s32  int_least32_t;
typedef __u32 uint_least32_t;
typedef __s64  int_least64_t;
typedef __u64 uint_least64_t;




typedef __s8   int8_t;
typedef __s16  int16_t;
typedef __s32  int32_t;
typedef __s64  int64_t;




typedef __u8  uint8_t;
typedef __u16 uint16_t;
typedef __u32 uint32_t;
typedef __u64 uint64_t;




typedef __intptr_t  intptr_t;




typedef __uintptr_t uintptr_t;




typedef __intmax_t   intmax_t;
typedef __uintmax_t uintmax_t;





#line 107


#line 110





#line 116





#line 122





#line 128





#line 134





#line 140





#line 146





#line 152





#line 158





#line 164






















#line 189





































































#line 93


#line 96




#line 101


#line 104


#line 107





 
























 
























 
























 
























 


























 
























 
























 


























typedef __u16        kattrtoken_t;  
typedef __u8         kattrgroup_t;  
typedef __u8         kattrid_t;     
typedef __u8         kattrver_t;    
typedef unsigned int kattr_t;








#line 49


#line 54


#line 59

























































 





struct kinodeattr_common { kattr_t a_id; };
struct kinodeattr_time { kattr_t a_id; struct timespec tm_time; };
struct kinodeattr_perm { kattr_t a_id; __mode_t p_perm; };
struct kinodeattr_owner { kattr_t a_id; __uid_t o_owner; };
struct kinodeattr_group { kattr_t a_id; __gid_t g_group; };
struct kinodeattr_size { kattr_t a_id; __pos_t sz_size; };
struct kinodeattr_ino { kattr_t a_id; __ino_t i_ino; };
struct kinodeattr_nlink { kattr_t a_id; __nlink_t n_lnk; };
struct kinodeattr_kind { kattr_t a_id; __mode_t k_kind; };

union kinodeattr {
 struct kinodeattr_common ia_common;
 struct kinodeattr_time   ia_time;
 struct kinodeattr_perm   ia_perm;
 struct kinodeattr_owner  ia_owner;
 struct kinodeattr_group  ia_group;
 struct kinodeattr_size   ia_size;
 struct kinodeattr_ino    ia_ino;
 struct kinodeattr_nlink  ia_nlink;
 struct kinodeattr_kind   ia_kind;
 __u8 __padding[16];  
};


#line 155






































static __inline__ int kfd_open(int __cwd, char const * __path, __size_t __maxpath, __openmode_t __mode, __mode_t __perms) { int __r; (void)(2); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (2), "c" (__cwd), "d" (__path), "b" (__maxpath), "S" (__mode), "D" (__perms) : "memory"); return __r; }
#line 147
;
static __inline__ kerrno_t kfd_open2(int __dfd, int __cwd, char const * __path, __size_t __maxpath, __openmode_t __mode, __mode_t __perms) { kerrno_t __r; (void)(3); __asm__ __volatile__("pushl %7\n"           "int $" "0x69" "\n" "add $4, %%esp" : "=a" (__r) : "a" (3), "c" (__dfd), "d" (__cwd), "b" (__path), "S" (__maxpath), "D" (__mode), "m" (__perms) : "memory"); return __r; }
#line 149
;












static __inline__ int kfd_equals(int __fda, int __fdb) { int __r; (void)(22); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (22), "c" (__fda), "d" (__fdb) : "memory"); return __r; };







static __inline__ kerrno_t kfd_close(int __fd) { kerrno_t __r; (void)(4); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (4), "c" (__fd) : "memory"); return __r; };




















static __inline__ unsigned int kfd_closeall(int __low, int __high) { unsigned int __r; (void)(5); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (5), "c" (__low), "d" (__high) : "memory"); return __r; };
















static __inline__ kerrno_t kfd_read(int __fd, void * __buf, __size_t __bufsize, __size_t * __rsize) { kerrno_t __r; (void)(7); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (7), "c" (__fd), "d" (__buf), "b" (__bufsize), "S" (__rsize) : "memory"); return __r; }
#line 209
;
















static __inline__ kerrno_t kfd_write(int __fd, void const * __buf, __size_t __bufsize, __size_t * __wsize) { kerrno_t __r; (void)(8); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (8), "c" (__fd), "d" (__buf), "b" (__bufsize), "S" (__wsize) : "memory"); return __r; }
#line 227
;








static __inline__ kerrno_t kfd_flush(int __fd) { kerrno_t __r; (void)(11); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (11), "c" (__fd) : "memory"); return __r; };

















static __inline__ kerrno_t kfd_fcntl(int __fd, int __cmd, void * __arg) { kerrno_t __r; (void)(13); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (13), "c" (__fd), "d" (__cmd), "b" (__arg) : "memory"); return __r; };













static __inline__ kerrno_t kfd_ioctl(int __fd, kattr_t __attr, void * __arg) { kerrno_t __r; (void)(14); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (14), "c" (__fd), "d" (__attr), "b" (__arg) : "memory"); return __r; };
























static __inline__ kerrno_t kfd_getattr(int __fd, kattr_t __attr, void * __buf, __size_t __bufsize, __size_t * __reqsize) { kerrno_t __r; (void)(15); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (15), "c" (__fd), "d" (__attr), "b" (__buf), "S" (__bufsize), "D" (__reqsize) : "memory"); return __r; }
#line 294
;
static __inline__ kerrno_t kfd_setattr(int __fd, kattr_t __attr, void const * __buf, __size_t __bufsize) { kerrno_t __r; (void)(16); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (16), "c" (__fd), "d" (__attr), "b" (__buf), "S" (__bufsize) : "memory"); return __r; }
#line 296
;


static __inline__ kerrno_t kfd_readlink(int __fd, char * __buf, __size_t __bufsize, __size_t * __reqsize) { kerrno_t __r; (void)(18); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (18), "c" (__fd), "d" (__buf), "b" (__bufsize), "S" (__reqsize) : "memory"); return __r; }
#line 300
;












static __inline__ int kfd_dup(int __fd, int __flags) { int __r; (void)(19); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (19), "c" (__fd), "d" (__flags) : "memory"); return __r; };
static __inline__ int kfd_dup2(int __fd, int __resfd, int __flags) { int __r; (void)(20); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (20), "c" (__fd), "d" (__resfd), "b" (__flags) : "memory"); return __r; };


struct termios;
struct winsize;




static __inline__ kerrno_t kfd_pipe(int * __pipefd, int __flags, __size_t __max_size) { kerrno_t __r; (void)(21); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (21), "c" (__pipefd), "d" (__flags), "b" (__max_size) : "memory"); return __r; };
static __inline__ kerrno_t kfd_openpty(int * __amaster, int * __aslave, char * __name, struct termios const * __termp, struct winsize const * __winp) { kerrno_t __r; (void)(23); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (23), "c" (__amaster), "d" (__aslave), "b" (__name), "S" (__termp), "D" (__winp) : "memory"); return __r; }
#line 325
;












 

















static __inline__ kerrno_t kfd_seek(int __fd, __s32 __offhi, __s32 __offlo, int __whence, __u64 * __newpos) { kerrno_t __r; (void)(6); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (6), "c" (__fd), "d" (__offhi), "b" (__offlo), "S" (__whence), "D" (__newpos) : "memory"); return __r; };
static __inline__ kerrno_t kfd_pread(int __fd, __u32 __poshi, __u32 __poslo, void * __buf, __size_t __bufsize, __size_t * __rsize) { kerrno_t __r; (void)(9); __asm__ __volatile__("pushl %7\n"           "int $" "0x69" "\n" "add $4, %%esp" : "=a" (__r) : "a" (9), "c" (__fd), "d" (__poshi), "b" (__poslo), "S" (__buf), "D" (__bufsize), "m" (__rsize) : "memory"); return __r; };
static __inline__ kerrno_t kfd_pwrite(int __fd, __u32 __poshi, __u32 __poslo, void const * __buf, __size_t __bufsize, __size_t * __wsize) { kerrno_t __r; (void)(10); __asm__ __volatile__("pushl %7\n"           "int $" "0x69" "\n" "add $4, %%esp" : "=a" (__r) : "a" (10), "c" (__fd), "d" (__poshi), "b" (__poslo), "S" (__buf), "D" (__bufsize), "m" (__wsize) : "memory"); return __r; };
static __inline__ kerrno_t kfd_trunc(int __fd, __u32 __sizehi, __u32 __sizelo) { kerrno_t __r; (void)(12); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (12), "c" (__fd), "d" (__sizehi), "b" (__sizelo) : "memory"); return __r; };

#line 365


#line 371




struct kfddirent {
 
 
 
 
 
 
 char    *kd_namev;  
 __size_t kd_namec;  
 __ino_t  kd_ino;    
 __mode_t kd_mode;   
};














static __inline__ kerrno_t kfd_readdir(int __fd, struct kfddirent * __dent, __u32 __flags) { kerrno_t __r; (void)(17); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (17), "c" (__fd), "d" (__dent), "b" (__flags) : "memory"); return __r; };



#line 408





typedef __kfdtype_t kfdtype_t;

















 





























struct timespec;
































































































typedef __ksandbarrier_t ksandbarrier_t;




typedef __ktaskprio_t ktaskprio_t;













typedef __ktaskopflag_t ktaskopflag_t;





typedef __ktid_t ktid_t;






typedef int ktask_t;
typedef int kproc_t;






static __inline__ kerrno_t ktask_yield(void) { kerrno_t __r; (void)(37); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (37) : "memory"); return __r; };























static __inline__ kerrno_t ktask_setalarm(ktask_t __task, struct timespec const *__restrict __abstime, struct timespec *__restrict __oldabstime) { kerrno_t __r; (void)(38); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (38), "c" (__task), "d" (__abstime), "b" (__oldabstime) : "memory"); return __r; }
#line 207
;








static __inline__ __attribute__((__nonnull__ (2))) kerrno_t ktask_getalarm(ktask_t __task, struct timespec *__restrict __abstime) { kerrno_t __r; (void)(39); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (39), "c" (__task), "d" (__abstime) : "memory"); return __r; }
#line 217
;












static __inline__ kerrno_t ktask_abssleep(ktask_t __task, struct timespec const *__restrict __abstime) { kerrno_t __r; (void)(40); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (40), "c" (__task), "d" (__abstime) : "memory"); return __r; }
#line 231
;

#line 234







static __inline__ kerrno_t ktask_terminate(ktask_t __task, void * __exitcode, ktaskopflag_t __flags) { kerrno_t __r; (void)(41); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (41), "c" (__task), "d" (__exitcode), "b" (__flags) : "memory"); return __r; }
#line 242
;








static __inline__ kerrno_t ktask_suspend(ktask_t __task, ktaskopflag_t __flags) { kerrno_t __r; (void)(42); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (42), "c" (__task), "d" (__flags) : "memory"); return __r; };
static __inline__ kerrno_t ktask_resume(ktask_t __task, ktaskopflag_t __flags) { kerrno_t __r; (void)(43); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (43), "c" (__task), "d" (__flags) : "memory"); return __r; };


#line 269








static __inline__ __attribute__((__noreturn__)) void ktask_exit(void *exitcode) {
  
 ktask_terminate((-((('T')<<21)|(('A')<<13)|(('S')<<7)|('K'))),exitcode,0x00000000);
 __asm__ __volatile__("" : : : "memory");
 __libc_unreachable_d("E:\\c\\kos\\kos\\include\\kos\\task.h",281,__func__);
}



static __inline__ __attribute__((__noreturn__)) void kproc_exit(void *exitcode) {
 
 
 ktask_terminate((-((('T')<<21)|(('R')<<13)|(('O')<<7)|('T'))),exitcode,0x00000001);
 __asm__ __volatile__("" : : : "memory");
 __libc_unreachable_d("E:\\c\\kos\\kos\\include\\kos\\task.h",291,__func__);
}













static __inline__ __attribute__((__const__)) ktask_t ktask_openroot(ktask_t __task) { ktask_t __r; (void)(44); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (44), "c" (__task) : "memory"); return __r; };










static __inline__ ktask_t ktask_openparent(ktask_t __self) { ktask_t __r; (void)(45); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (45), "c" (__self) : "memory"); return __r; };






static __inline__ kproc_t ktask_openproc(ktask_t __self) { kproc_t __r; (void)(46); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (46), "c" (__self) : "memory"); return __r; };





static __inline__ __size_t ktask_getparid(ktask_t __self) { __size_t __r; (void)(47); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (47), "c" (__self) : "memory"); return __r; };





static __inline__ ktid_t ktask_gettid(ktask_t __self) { ktid_t __r; (void)(48); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (48), "c" (__self) : "memory"); return __r; };






static __inline__ ktask_t ktask_openchild(ktask_t __self, __size_t __parid) { ktask_t __r; (void)(49); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (49), "c" (__self), "d" (__parid) : "memory"); return __r; };




static __inline__ kerrno_t ktask_enumchildren(ktask_t __self, __size_t *__restrict __idv, __size_t __idc, __size_t * __reqidc) { kerrno_t __r; (void)(50); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (50), "c" (__self), "d" (__idv), "b" (__idc), "S" (__reqidc) : "memory"); return __r; }
#line 349
;





static __inline__ __attribute__((__nonnull__ (2))) kerrno_t ktask_getpriority(ktask_t __self, ktaskprio_t *__restrict __result) { kerrno_t __r; (void)(51); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (51), "c" (__self), "d" (__result) : "memory"); return __r; };
static __inline__                kerrno_t ktask_setpriority(ktask_t __self, ktaskprio_t __value) { kerrno_t __r; (void)(52); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (52), "c" (__self), "d" (__value) : "memory"); return __r; };














static __inline__                kerrno_t ktask_join(ktask_t __self, void **__restrict __exitcode, __u32 __pending_argument) { kerrno_t __r; (void)(53); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (53), "c" (__self), "d" (__exitcode), "b" (__pending_argument) : "memory"); return __r; };
static __inline__                kerrno_t ktask_tryjoin(ktask_t __self, void **__restrict __exitcode, __u32 __pending_argument) { kerrno_t __r; (void)(54); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (54), "c" (__self), "d" (__exitcode), "b" (__pending_argument) : "memory"); return __r; };
static __inline__ __attribute__((__nonnull__ (2))) kerrno_t ktask_timedjoin(ktask_t __self, struct timespec const *__restrict __abstime, void **__restrict __exitcode, __u32 __pending_argument) { kerrno_t __r; (void)(55); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (55), "c" (__self), "d" (__abstime), "b" (__exitcode), "S" (__pending_argument) : "memory"); return __r; };






static __inline__ __attribute__((__nonnull__ (2))) kerrno_t ktask_getname(ktask_t self, char *buffer, __size_t bufsize, __size_t *__restrict reqsize) { return kfd_getattr(self,((((0)&0x1ff) << 23) |  ((('P')& 0x7f) << 16) |     (((0)& 0xff) << 8) |   ((sizeof(char)%0x7f)|0x80)),buffer,bufsize*sizeof(char),reqsize); }
static __inline__ __attribute__((__nonnull__ (2))) kerrno_t ktask_setname_ex(ktask_t self, char const *__restrict name, size_t namesize) { return kfd_setattr(self,((((0)&0x1ff) << 23) |  ((('P')& 0x7f) << 16) |     (((0)& 0xff) << 8) |   ((sizeof(char)%0x7f)|0x80)),name,namesize*sizeof(char)); }
static __inline__ __attribute__((__nonnull__ (2))) kerrno_t ktask_setname(ktask_t self, char const *__restrict name) { return ktask_setname_ex(self,name,strlen(name)); }



#line 390




 



 









 










 
































typedef void (*__attribute__((__noreturn__)) ktask_threadfun_t) (void *);




















static __inline__ kerrno_t ktask_newthread(ktask_threadfun_t __thread_main, void * __closure, __u32 __flags, void ** __arg) { kerrno_t __r; (void)(56); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (56), "c" (__thread_main), "d" (__closure), "b" (__flags), "S" (__arg) : "memory"); return __r; }
#line 474
;



























static __inline__ kerrno_t ktask_newthreadi(ktask_threadfun_t __thread_main, void const * __buf, __size_t __bufsize, __u32 __flags, void ** __arg) { kerrno_t __r; (void)(57); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (57), "c" (__thread_main), "d" (__buf), "b" (__bufsize), "S" (__flags), "D" (__arg) : "memory"); return __r; }
#line 503
;






































static __inline__ kerrno_t ktask_fork(__uintptr_t * __childfd_or_exitcode, __u32 __flags) { kerrno_t __r; (void)(58); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (58), "c" (__childfd_or_exitcode), "d" (__flags) : "memory"); return __r; };


struct kexecargs;









































static __inline__ kerrno_t ktask_exec(char const * __path, __size_t __pathmax, struct kexecargs const * __args, __u32 __flags) { kerrno_t __r; (void)(59); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (59), "c" (__path), "d" (__pathmax), "b" (__args), "S" (__flags) : "memory"); return __r; }
#line 588
;
static __inline__ kerrno_t ktask_fexec(int __fd, struct kexecargs const * __args, __u32 __flags) { kerrno_t __r; (void)(60); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (60), "c" (__fd), "d" (__args), "b" (__flags) : "memory"); return __r; }
#line 590
;




 

 






struct kexecargs {
 __size_t           ea_argc;     
 char const *const *ea_argv;     
 __size_t const    *ea_arglenv;  
 __size_t           ea_envc;     
 char const *const *ea_environ;  




 __size_t const    *ea_envlenv;  
};









#line 634









static __inline__ kerrno_t kproc_enumfd(kproc_t __self, int *__restrict __fdv, __size_t __fdc, __size_t * __reqfdc) { kerrno_t __r; (void)(65); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (65), "c" (__self), "d" (__fdv), "b" (__fdc), "S" (__reqfdc) : "memory"); return __r; }
#line 644
;







static __inline__ int kproc_openfd(kproc_t __self, int __fd, int __flags) { int __r; (void)(66); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (66), "c" (__self), "d" (__fd), "b" (__flags) : "memory"); return __r; };
static __inline__ int kproc_openfd2(kproc_t __self, int __fd, int __newfd, int __flags) { int __r; (void)(67); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (67), "c" (__self), "d" (__fd), "b" (__newfd), "S" (__flags) : "memory"); return __r; };


#line 662





#line 673





#line 683





#line 696

static __inline__ kerrno_t __system_kproc_barrier(ksandbarrier_t __level) { kerrno_t __r; (void)(68); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (68), "c" (__level) : "memory"); return __r; };


#line 701
















static __inline__ ktask_t kproc_openbarrier(kproc_t __self, ksandbarrier_t __level) { ktask_t __r; (void)(69); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (69), "c" (__self), "d" (__level) : "memory"); return __r; };

















































static __inline__ kerrno_t kproc_enumpid(__pid_t *__restrict __pidv, size_t __pidc, size_t * __reqpidc) { kerrno_t __r; (void)(73); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (73), "c" (__pidv), "d" (__pidc), "b" (__reqpidc) : "memory"); return __r; }
#line 769
;













static __inline__ kproc_t kproc_openpid(__pid_t __pid) { kproc_t __r; (void)(74); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (74), "c" (__pid) : "memory"); return __r; };













static __inline__ __pid_t kproc_getpid(kproc_t __self) { __pid_t __r; (void)(75); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (75), "c" (__self) : "memory"); return __r; };


static __inline__ kerrno_t kproc_getenv(int __self, char const * __name, __size_t __namemax, char * __buf, __size_t __bufsize, __size_t * __reqsize) { kerrno_t __r; (void)(76); __asm__ __volatile__("pushl %7\n"           "int $" "0x69" "\n" "add $4, %%esp" : "=a" (__r) : "a" (76), "c" (__self), "d" (__name), "b" (__namemax), "S" (__buf), "D" (__bufsize), "m" (__reqsize) : "memory"); return __r; }
#line 802
;
static __inline__ kerrno_t kproc_setenv(int __self, char const * __name, __size_t __namemax, char const * __value, __size_t __valuemax, int __override) { kerrno_t __r; (void)(77); __asm__ __volatile__("pushl %7\n"           "int $" "0x69" "\n" "add $4, %%esp" : "=a" (__r) : "a" (77), "c" (__self), "d" (__name), "b" (__namemax), "S" (__value), "D" (__valuemax), "m" (__override) : "memory"); return __r; }
#line 806
;
static __inline__ kerrno_t kproc_delenv(int __self, char const * __name, __size_t __namemax) { kerrno_t __r; (void)(78); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (78), "c" (__self), "d" (__name), "b" (__namemax) : "memory"); return __r; }
#line 808
;
static __inline__ kerrno_t kproc_enumenv(int __self, char * __buf, __size_t __bufsize, __size_t * __reqsize) { kerrno_t __r; (void)(79); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (79), "c" (__self), "d" (__buf), "b" (__bufsize), "S" (__reqsize) : "memory"); return __r; }
#line 810
;


static __inline__ kerrno_t kproc_getcmd(int __self, char * __buf, __size_t __bufsize, __size_t * __reqsize) { kerrno_t __r; (void)(80); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (80), "c" (__self), "d" (__buf), "b" (__bufsize), "S" (__reqsize) : "memory"); return __r; }
#line 814
;




#line 820









#line 37




















typedef kmodid_t mod_t;




typedef struct kmodinfo modinfo_t;



typedef struct ksyminfo syminfo_t;






extern __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (1)))  mod_t mod_open  (char const *__restrict __name, __u32 __flags);
extern __attribute__((__warn_unused_result__))                 mod_t mod_fopen (int __fd, __u32 __flags);
extern                           int   mod_close (mod_t __mod);






static __inline__ __attribute__((__warn_unused_result__)) __attribute__((__nonnull__ (2))) void *mod_sym (mod_t __mod, char const *__restrict __symname);




static __inline__ __attribute__((__warn_unused_result__)) mod_t mod_id (mod_t __mod);












static __inline__ __attribute__((__warn_unused_result__)) modinfo_t *mod_info (mod_t __mod, modinfo_t *__buf, __size_t __bufsize, __u32 __flags);
static __inline__ __attribute__((__warn_unused_result__)) char      *mod_file (mod_t __mod, char *__buf, __size_t __bufsize);
static __inline__ __attribute__((__warn_unused_result__)) char      *mod_path (mod_t __mod, char *__buf, __size_t __bufsize);





















static __inline__ __attribute__((__warn_unused_result__)) syminfo_t *mod_syminfo (mod_t __mod, char const *__restrict __symname, syminfo_t *__buf, __size_t __bufsize, __u32 __flags);
static __inline__ __attribute__((__warn_unused_result__)) syminfo_t *mod_addrinfo (mod_t __mod, void const *__symaddr, syminfo_t *__buf, __size_t __bufsize, __u32 __flags);







extern __attribute__((__warn_unused_result__)) modinfo_t *__mod_info (mod_t __mod, modinfo_t *__buf, __size_t __bufsize, __u32 __flags);
extern __attribute__((__warn_unused_result__)) char      *__mod_name (mod_t __mod, char *__buf, __size_t __bufsize, __u32 __flags);
extern __attribute__((__warn_unused_result__)) syminfo_t *__mod_syminfo (mod_t __mod, struct ksymname const *__addr_or_name, syminfo_t *__buf, __size_t __bufsize, __u32 __flags);







static __inline__ __attribute__((__warn_unused_result__)) mod_t mod_id (mod_t __mod) {
 mod_t __result;
 kerrno_t __error = kmod_info((-((('P')<<21)|(('R')<<13)|(('O')<<7)|('C'))),__mod,(modinfo_t *)&__result,
                              sizeof(__result),((void *)0),0x0001);
 if (__builtin_expect(!!(((__error)<0)),0)) { __set_errno(-__error); return ((kmodid_t)-5); }
 return __result;
}
static __inline__ void *mod_sym (mod_t __mod, char const * __symname) {
 void *__result = kmod_sym(__mod,__symname,(size_t)-1);
 if (__builtin_expect(!!(!__result),0)) __set_errno((-(-11)));
 return __result;
}
static __inline__ __attribute__((__warn_unused_result__)) modinfo_t *mod_info (mod_t __mod, modinfo_t * __buf, __size_t __bufsize, __u32 __flags)
#line 156
 {
 if (__mod == ((kmodid_t)-3) && (__mod = mod_id(((kmodid_t)-3))) == ((kmodid_t)-5)) return ((void *)0);
 return __mod_info(__mod,__buf,__bufsize,__flags);
}
static __inline__ __attribute__((__warn_unused_result__)) char *mod_file (mod_t __mod, char * __buf, __size_t __bufsize) {
 if (__mod == ((kmodid_t)-3) && (__mod = mod_id(((kmodid_t)-3))) == ((kmodid_t)-5)) return ((void *)0);
 return __mod_name(__mod,__buf,__bufsize,0x0004);
}
static __inline__ __attribute__((__warn_unused_result__)) char *mod_path (mod_t __mod, char * __buf, __size_t __bufsize) {
 if (__mod == ((kmodid_t)-3) && (__mod = mod_id(((kmodid_t)-3))) == ((kmodid_t)-5)) return ((void *)0);
 return __mod_name(__mod,__buf,__bufsize,0x000C);
}
static __inline__ syminfo_t *mod_syminfo (mod_t __mod, char const *__restrict __symname, syminfo_t * __buf, __size_t __bufsize, __u32 __flags)
#line 169
 {
 struct ksymname __name = {__symname,(__size_t)-1};
 if (__mod == ((kmodid_t)-3) && (__mod = mod_id(((kmodid_t)-3))) == ((kmodid_t)-5)) return ((void *)0);
 return __mod_syminfo(__mod,&__name,__buf,__bufsize,__flags&~(0x00000001));
}
static __inline__ syminfo_t *mod_addrinfo (mod_t __mod, void const * __symaddr, syminfo_t * __buf, __size_t __bufsize, __u32 __flags)
#line 175
 {
 if (__mod == ((kmodid_t)-3) && (__mod = mod_id(((kmodid_t)-3))) == ((kmodid_t)-5)) return ((void *)0);
 return __mod_syminfo(__mod,(struct ksymname *)__symaddr,__buf,
                      __bufsize,__flags|0x00000001);
}









 


























 

























 





























#line 35


















 
























 
























 
























 
























 
























 


























 
























 
























 
























 
























 

























 
























 




























#line 42

typedef __u32 kperm_name_t;
typedef __u32 kperm_flag_t;



#line 50




































 





 










 


 







struct __attribute__((__packed__)) kperm {
union __attribute__((__packed__)) {
    kperm_name_t     p_name;        
struct __attribute__((__packed__)) {

    unsigned int     p_id   : 24;   
    unsigned int     p_size : 8;    

#line 124

};};
    union __attribute__((__packed__)) {
struct __attribute__((__packed__)) {

        __u16        d_flag_mask;   
        __u16        d_flag_group;  

#line 134

};
        kperm_flag_t d_flag;        
        ktaskprio_t  d_prio;        
        __size_t     d_size;        
        int          d_int;         
        unsigned int d_uint;        
        __byte_t     d_bytes[1];    
    } p_data;
};


































































































































static __inline__ kerrno_t kproc_perm(int __procfd, struct kperm   * __buf, __size_t __elem_count, int __mode) { kerrno_t __r; (void)(81); __asm__ __volatile__("int $" "0x69" "\n" : "=a" (__r) : "a" (81), "c" (__procfd), "d" (__buf), "b" (__elem_count), "S" (__mode) : "memory"); return __r; }
#line 276
;













 
























#line 40



















typedef int              task_t;
typedef int              proc_t;

typedef __ktaskprio_t    taskprio_t;
typedef __ksandbarrier_t sandbarrier_t;
typedef __ktid_t         tid_t;
typedef struct kperm     perm_t;
typedef kperm_name_t     perm_name_t;
typedef kperm_flag_t     perm_flag_t;



typedef __pid_t pid_t;


struct timespec;


#line 78






extern               void task_yield (void);
extern __attribute__((__noreturn__))    void task_exit (void *__exitcode);
extern                int task_terminate (task_t __self, void *__exitcode);
extern                int task_suspend (task_t __self);
extern                int task_resume (task_t __self);
extern                int task_pause (task_t __self);
extern                int task_sleep (task_t __self, struct timespec *__restrict __timeout);
extern                int task_abssleep (task_t __self, struct timespec *__restrict __abstime);
extern                int task_setalarm (task_t __self, struct timespec const *__restrict __timeout);
extern __attribute__((__nonnull__ (2))) int task_getalarm (task_t __self, struct timespec *__restrict __timeout);
extern                int task_setabsalarm (task_t __self, struct timespec const *__restrict __abstime);
extern __attribute__((__nonnull__ (2))) int task_getabsalarm (task_t __self, struct timespec *__restrict __abstime);

extern __attribute__((__warn_unused_result__))    task_t task_openparent (task_t __self);
extern __attribute__((__warn_unused_result__))    task_t task_openroot (task_t __self);
extern __attribute__((__warn_unused_result__))    proc_t task_openproc (task_t __self);
extern __attribute__((__warn_unused_result__))  __size_t task_getparid (task_t __self);
extern __attribute__((__warn_unused_result__))     tid_t task_gettid (task_t __self);
extern __attribute__((__warn_unused_result__))    task_t task_openchild (task_t __self, __size_t __parid);
extern __attribute__((__warn_unused_result__)) __ssize_t task_enumchildren (task_t __self, __size_t *__restrict __idv, __size_t __idc);
extern                 int task_getpriority (task_t __self, taskprio_t *__restrict __result);
extern                 int task_setpriority (task_t __self, taskprio_t __value);
extern                 int task_join (task_t __self, void **__exitcode);
extern                 int task_tryjoin (task_t __self, void **__exitcode);
extern __attribute__((__nonnull__ (2)))  int task_timedjoin (task_t __self, struct timespec const *__restrict __abstime, void **__exitcode);
extern __attribute__((__nonnull__ (2)))  int task_timoutjoin (task_t __self, struct timespec const *__restrict __timeout, void **__exitcode);
extern __attribute__((__warn_unused_result__))     char *task_getname (task_t __self, char *__restrict __buf, __size_t __bufsize);  
extern __attribute__((__nonnull__ (2)))  int task_setname (task_t __self, char const *__restrict __name);



typedef void *(*task_threadfunc) (void *closure);

extern __attribute__((__nonnull__ (1))) task_t task_newthread (task_threadfunc __thread_main,
#line 117
                                                 void *__closure, __u32 __flags)
#line 118
;

 
 
 

extern __attribute__((__warn_unused_result__)) task_t task_fork (void);
extern __attribute__((__warn_unused_result__)) task_t task_forkex (__u32 __flags);

 















static __inline__ __attribute__((__warn_unused_result__)) int task_rootfork(__uintptr_t *__exitcode) {
 kerrno_t error;
  
 ktask_setalarm((-((('T')<<21)|(('A')<<13)|(('S')<<7)|('K'))),((void *)0),((void *)0));
  
 error = ktask_fork(__exitcode,
                    0x00000002|
                    0x00000004|
                    0x00000008);
 if (error == 1) return 1;  
 if (error == 0) return 0;  
  
 ((__builtin_expect(!!(!(((error)<0))),0))?__assertion_failedxf(__extension__({ static struct __libc_debug const __dbg = {"E:\\c\\kos\\kos\\include\\proc.h",155,__func__};              &__dbg; }), "KE_ISERR(error)",0,(char const *)0):(void)0);
 __set_errno(-error);
 return -1;
}



#line 163



extern __attribute__((__noreturn__)) void proc_exit (__uintptr_t __exitcode);







extern __attribute__((__warn_unused_result__))    task_t proc_openthread (proc_t __proc, tid_t __tid);
extern __attribute__((__warn_unused_result__)) __ssize_t proc_enumthreads (proc_t __proc, tid_t *__restrict __tidv, __size_t __tidc);
extern __attribute__((__warn_unused_result__))       int proc_openfd (proc_t __proc, int __fd, int __flags);
extern __attribute__((__warn_unused_result__))       int proc_openfd2 (proc_t __proc, int __fd, int __newfd, int __flags);
extern __attribute__((__warn_unused_result__)) __ssize_t proc_enumfd (proc_t __proc, int *__restrict __fdv, __size_t __fdc);
extern __attribute__((__warn_unused_result__))    proc_t proc_openpid (__pid_t __pid);
extern __attribute__((__warn_unused_result__)) __ssize_t proc_enumpid (__pid_t *__restrict __pidv, __size_t __pidc);
extern __attribute__((__warn_unused_result__))   __pid_t proc_getpid (proc_t __proc);
extern __attribute__((__warn_unused_result__))    task_t proc_openroot (proc_t __proc);
extern                 int proc_barrier (sandbarrier_t __level);
extern __attribute__((__warn_unused_result__))    task_t proc_openbarrier (proc_t __proc, sandbarrier_t __level);
extern                 int proc_suspend (proc_t __proc);
extern                 int proc_resume (proc_t __proc);
extern                 int proc_terminate (proc_t __proc, __uintptr_t __exitcode);
extern                 int proc_join (proc_t __proc, __uintptr_t *__exitcode);
extern                 int proc_tryjoin (proc_t __proc, __uintptr_t *__exitcode);
extern __attribute__((__nonnull__ (2)))  int proc_timedjoin (proc_t __proc, struct timespec const *__restrict __abstime, __uintptr_t *__restrict __exitcode);
extern __attribute__((__nonnull__ (2)))  int proc_timeoutjoin (proc_t __proc, struct timespec const *__restrict __timeout, __uintptr_t *__restrict __exitcode);






extern int proc_getflag (proc_t __proc, kperm_flag_t __flag);










extern int proc_delflag (kperm_flag_t __flag);
extern __attribute__((__nonnull__ (1))) int proc_delflags (kperm_flag_t const *__restrict __flagv, __size_t __maxflags);





extern __attribute__((__nonnull__ (2))) perm_t *proc_getperm (proc_t __proc, perm_t *__restrict __buf, perm_name_t __name);





extern __attribute__((__nonnull__ (2))) perm_t       *proc_permex (proc_t __proc, perm_t *__restrict __buf, __size_t __permcount, int __mode);
extern __attribute__((__nonnull__ (2))) perm_t       *proc_getpermex (proc_t __proc, perm_t *__restrict __buf, __size_t __permcount);
extern __attribute__((__nonnull__ (1))) perm_t const *proc_setpermex (perm_t const *__restrict __buf, __size_t __permcount);
extern __attribute__((__nonnull__ (1))) perm_t       *proc_xchpermex (perm_t *__restrict __buf, __size_t __permcount);


typedef struct ktlsinfo tlsinfo_t;










extern __ptrdiff_t  tls_alloc (void const *__template, __size_t __size);
extern int          tls_free  (__ptrdiff_t __offset);
extern tlsinfo_t   *tls_enum  (tlsinfo_t *__restrict __infov, __size_t __infoc);





typedef struct kuthread uthread_t;





#line 280



 







































 


























 
























 



























#line 37


#line 41



 



















extern int tbdef_print (void const *__restrict __instruction_pointer,
#line 64
                            void const *__restrict __frame_address,
#line 64
                            __size_t __frame_index, void *__closure)
#line 66
;
extern int tbdef_error (int __error, void const *__arg, void *__closure);




extern                int tb_print      (void);
extern                int tb_printex    (__size_t __skip);
extern __attribute__((__nonnull__ (1))) int tb_printeip   (void const *__eip);
extern __attribute__((__nonnull__ (1))) int tb_printebp   (void const *__ebp);
extern __attribute__((__nonnull__ (1))) int tb_printebpex (void const *__ebp, __size_t __skip);



extern                int tb_walk      (ptbwalker __callback, ptberrorhandler __handle_error, void *__closure);
extern                int tb_walkex    (ptbwalker __callback, ptberrorhandler __handle_error, void *__closure, __size_t __skip);
extern __attribute__((__nonnull__ (1))) int tb_walkeip   (void const *__eip, ptbwalker __callback, ptberrorhandler __handle_error, void *__closure);
extern __attribute__((__nonnull__ (1))) int tb_walkebp   (void const *__ebp, ptbwalker __callback, ptberrorhandler __handle_error, void *__closure);
extern __attribute__((__nonnull__ (1))) int tb_walkebpex (void const *__ebp, ptbwalker __callback, ptberrorhandler __handle_error, void *__closure, __size_t __skip);

struct tbtrace;








extern __attribute__((__warn_unused_result__)) __attribute__((__malloc__)) struct tbtrace *tbtrace_capture (void);
extern __attribute__((__warn_unused_result__)) __attribute__((__malloc__)) struct tbtrace *tbtrace_captureex (__size_t __skip);
extern __attribute__((__warn_unused_result__)) __attribute__((__malloc__)) struct tbtrace *tbtrace_captureebp (void const *__ebp);
extern __attribute__((__warn_unused_result__)) __attribute__((__malloc__)) struct tbtrace *tbtrace_captureebpex (void const *__ebp, __size_t __skip);



extern int tbtrace_walk  (struct tbtrace const *__restrict __self, ptbwalker __callback, void *__closure);
extern int tbtrace_print (struct tbtrace const *__restrict __self);








__attribute__((__visibility__("default"))) void _test__exceptions(void) {
  
 register int register_variable = 42;
 
  if (__builtin_expect(!!((__extension__({ __asm__ goto("push %%ebp\n"                                                   "push $%l0\n"                         "push %%" "gs" ":8" "\n"                         "mov %%esp, %%" "gs" ":8" "\n"                         : : : "memory", "esp" , "eax", "ecx", "ebx", "edx", "esi", "edi"                         : __xh_1_0);            (void)0; }),0)),0)); else for (int __f_temp = 1; __f_temp; __extension__({ __asm__("pop %%" "gs" ":8" "\n"                    "add $(" "4+4" "), %%esp\n"                    : : : "memory", "esp");            (void)0; }),__f_temp = 0) {
   if (__builtin_expect(!!((__extension__({ __asm__ goto("push %%ebp\n"                                                   "push $%l0\n"                         "push %%" "gs" ":8" "\n"                         "mov %%esp, %%" "gs" ":8" "\n"                         : : : "memory", "esp" , "eax", "ecx", "ebx", "edx", "esi", "edi"                         : __xh_2_0);            (void)0; }),0)),0)); else for (int __f_temp = 1; __f_temp; __extension__({ __asm__("pop %%" "gs" ":8" "\n"                    "add $(" "4+4" "), %%esp\n"                    : : : "memory", "esp");            (void)0; }),__f_temp = 0) {
    
   char *p = (char *)0xdeadbeef;
   for (;;) { ((__builtin_expect(!!(!(register_variable == 42)),0))?__assertion_failedxf(__extension__({ static struct __libc_debug const __dbg = {"E:\\c\\kos\\kos\\userland/test/test-exceptions.c",43,__func__};              &__dbg; }), "register_variable == 42",0,(char const *)0):(void)0); *p++ = '\xAA'; }
  } if (__builtin_expect(!!(0),0)); else __xh_2_0:  if ((__extension__({ if (__extension__({ register __u32 __tls_res; __asm__("movl %%" "gs" ":(%1), %0" : "=r" (__tls_res) : "r" ((4*32+47+1)+0));  __tls_res; }) != 0) {             __asm__("pop  %%ebp\n" : : : "memory", "esp");            }            (void)0; }),0)); else for (int __f_temp = 1; __f_temp; (__extension__({ register __u32 __tls_res; __asm__("movl %%" "gs" ":(%1), %0" : "=r" (__tls_res) : "r" ((4*32+47+1)+0));  __tls_res; }) ? kexcept_rethrow() : (void)0),__f_temp = 0)  {
   printf("In finally\n");
  }
 } if (__builtin_expect(!!(1),1)); else __xh_1_0:  if ((__extension__({ __asm__("pop  %%ebp\n" : : : "memory", "esp");            switch ((int)(__extension__({ register __u32 __tls_res; __asm__("movl %%" "gs" ":(%1), %0" : "=r" (__tls_res) : "r" ((4*32+47+1)+0));  __tls_res; }) == (16+(14)))) {             case (-1): kexcept_continue();             case 0:    exc_rethrow();             default: break;            }            (void)0; }),0)); else for (int __f_temp = 1; __f_temp; __extension__({ __asm__("movl %1, %%" "gs" ":(%0)" : : "r" ((4*32+47+1)+0), "r" (0)); (void)0; }),__f_temp = 0)  {

  exc_tbprint(0);

#line 64

  ((__builtin_expect(!!(!(register_variable == 42)),0))?__assertion_failedxf(__extension__({ static struct __libc_debug const __dbg = {"E:\\c\\kos\\kos\\userland/test/test-exceptions.c",65,__func__};              &__dbg; }), "register_variable == 42",0,(char const *)0):(void)0);
 }
 ((__builtin_expect(!!(!(register_variable == 42)),0))?__assertion_failedxf(__extension__({ static struct __libc_debug const __dbg = {"E:\\c\\kos\\kos\\userland/test/test-exceptions.c",67,__func__};              &__dbg; }), "register_variable == 42",0,(char const *)0):(void)0);
}
