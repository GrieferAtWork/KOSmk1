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
#ifndef __KOS_SYSCONF_H__
#define __KOS_SYSCONF_H__ 1

/* Values for sysconf. */
#define _SC_ARG_MAX                      0
//#define _SC_CHILD_MAX                    1
//#define _SC_CLK_TCK                      2
//#define _SC_NGROUPS_MAX                  3
#define _SC_OPEN_MAX                     4
//#define _SC_STREAM_MAX                   5
//#define _SC_TZNAME_MAX                   6
//#define _SC_JOB_CONTROL                  7
//#define _SC_SAVED_IDS                    8
//#define _SC_REALTIME_SIGNALS             9
//#define _SC_PRIORITY_SCHEDULING          10
//#define _SC_TIMERS                       11
//#define _SC_ASYNCHRONOUS_IO              12
//#define _SC_PRIORITIZED_IO               13
//#define _SC_SYNCHRONIZED_IO              14
//#define _SC_FSYNC                        15
//#define _SC_MAPPED_FILES                 16
//#define _SC_MEMLOCK                      17
//#define _SC_MEMLOCK_RANGE                18
//#define _SC_MEMORY_PROTECTION            19
//#define _SC_MESSAGE_PASSING              20
//#define _SC_SEMAPHORES                   21
//#define _SC_SHARED_MEMORY_OBJECTS        22
//#define _SC_AIO_LISTIO_MAX               23
//#define _SC_AIO_MAX                      24
//#define _SC_AIO_PRIO_DELTA_MAX           25
//#define _SC_DELAYTIMER_MAX               26
//#define _SC_MQ_OPEN_MAX                  27
//#define _SC_MQ_PRIO_MAX                  28
//#define _SC_VERSION                      29
#define _SC_PAGESIZE                     30
#define _SC_PAGE_SIZE                    _SC_PAGESIZE
//#define _SC_RTSIG_MAX                    31
//#define _SC_SEM_NSEMS_MAX                32
//#define _SC_SEM_VALUE_MAX                33
//#define _SC_SIGQUEUE_MAX                 34
//#define _SC_TIMER_MAX                    35
//#define _SC_BC_BASE_MAX                  36
//#define _SC_BC_DIM_MAX                   37
//#define _SC_BC_SCALE_MAX                 38
//#define _SC_BC_STRING_MAX                39
//#define _SC_COLL_WEIGHTS_MAX             40
//#define _SC_EQUIV_CLASS_MAX              41
//#define _SC_EXPR_NEST_MAX                42
//#define _SC_LINE_MAX                     43
//#define _SC_RE_DUP_MAX                   44
//#define _SC_CHARCLASS_NAME_MAX           45
//#define _SC_2_VERSION                    46
//#define _SC_2_C_BIND                     47
//#define _SC_2_C_DEV                      48
//#define _SC_2_FORT_DEV                   49
//#define _SC_2_FORT_RUN                   50
//#define _SC_2_SW_DEV                     51
//#define _SC_2_LOCALEDEF                  52
//#define _SC_PII                          53
//#define _SC_PII_XTI                      54
//#define _SC_PII_SOCKET                   55
//#define _SC_PII_INTERNET                 56
//#define _SC_PII_OSI                      57
//#define _SC_POLL                         58
//#define _SC_SELECT                       59
//#define _SC_UIO_MAXIOV                   60
//#define _SC_IOV_MAX                      61
//#define _SC_PII_INTERNET_STREAM          62
//#define _SC_PII_INTERNET_DGRAM           63
//#define _SC_PII_OSI_COTS                 64
//#define _SC_PII_OSI_CLTS                 65
//#define _SC_PII_OSI_M                    66
//#define _SC_T_IOV_MAX                    67
//#define _SC_THREADS                      68
//#define _SC_THREAD_SAFE_FUNCTIONS        69
//#define _SC_GETGR_R_SIZE_MAX             70
//#define _SC_GETPW_R_SIZE_MAX             71
//#define _SC_LOGIN_NAME_MAX               72
//#define _SC_TTY_NAME_MAX                 73
//#define _SC_THREAD_DESTRUCTOR_ITERATIONS 74
//#define _SC_THREAD_KEYS_MAX              75
//#define _SC_THREAD_STACK_MIN             76
//#define _SC_THREAD_THREADS_MAX           77
//#define _SC_THREAD_ATTR_STACKADDR        78
//#define _SC_THREAD_ATTR_STACKSIZE        79
//#define _SC_THREAD_PRIORITY_SCHEDULING   80
//#define _SC_THREAD_PRIO_INHERIT          81
//#define _SC_THREAD_PRIO_PROTECT          82
//#define _SC_THREAD_PROCESS_SHARED        83
#define _SC_NPROCESSORS_CONF             84
#define _SC_NPROCESSORS_ONLN             85
//#define _SC_PHYS_PAGES                   86
#define _SC_AVPHYS_PAGES                 87
//#define _SC_ATEXIT_MAX                   88
//#define _SC_PASS_MAX                     89
//#define _SC_XOPEN_VERSION                90
//#define _SC_XOPEN_XCU_VERSION            91
//#define _SC_XOPEN_UNIX                   92
//#define _SC_XOPEN_CRYPT                  93
//#define _SC_XOPEN_ENH_I18N               94
//#define _SC_XOPEN_SHM                    95
//#define _SC_2_CHAR_TERM                  96
//#define _SC_2_C_VERSION                  97
//#define _SC_2_UPE                        98
//#define _SC_XOPEN_XPG2                   99
//#define _SC_XOPEN_XPG3                   100
//#define _SC_XOPEN_XPG4                   101
//#define _SC_CHAR_BIT                     102
//#define _SC_CHAR_MAX                     103
//#define _SC_CHAR_MIN                     104
//#define _SC_INT_MAX                      105
//#define _SC_INT_MIN                      106
//#define _SC_LONG_BIT                     107
//#define _SC_WORD_BIT                     108
//#define _SC_MB_LEN_MAX                   109
//#define _SC_NZERO                        110
//#define _SC_SSIZE_MAX                    111
//#define _SC_SCHAR_MAX                    112
//#define _SC_SCHAR_MIN                    113
//#define _SC_SHRT_MAX                     114
//#define _SC_SHRT_MIN                     115
//#define _SC_UCHAR_MAX                    116
//#define _SC_UINT_MAX                     117
//#define _SC_ULONG_MAX                    118
//#define _SC_USHRT_MAX                    119
//#define _SC_NL_ARGMAX                    120
//#define _SC_NL_LANGMAX                   121
//#define _SC_NL_MSGMAX                    122
//#define _SC_NL_NMAX                      123
//#define _SC_NL_SETMAX                    124
//#define _SC_NL_TEXTMAX                   125
//#define _SC_XBS5_ILP32_OFF32             126
//#define _SC_XBS5_ILP32_OFFBIG            127
//#define _SC_XBS5_LP64_OFF64              128
//#define _SC_XBS5_LPBIG_OFFBIG            129
//#define _SC_XOPEN_LEGACY                 130
//#define _SC_XOPEN_REALTIME               131
//#define _SC_XOPEN_REALTIME_THREADS       132
//#define _SC_ADVISORY_INFO                133
//#define _SC_BARRIERS                     134
//#define _SC_BASE                         135
//#define _SC_C_LANG_SUPPORT               136
//#define _SC_C_LANG_SUPPORT_R             137
//#define _SC_CLOCK_SELECTION              138
//#define _SC_CPUTIME                      139
//#define _SC_THREAD_CPUTIME               140
//#define _SC_DEVICE_IO                    141
//#define _SC_DEVICE_SPECIFIC              142
//#define _SC_DEVICE_SPECIFIC_R            143
//#define _SC_FD_MGMT                      144
//#define _SC_FIFO                         145
//#define _SC_PIPE                         146
//#define _SC_FILE_ATTRIBUTES              147
//#define _SC_FILE_LOCKING                 148
//#define _SC_FILE_SYSTEM                  149
//#define _SC_MONOTONIC_CLOCK              150
//#define _SC_MULTI_PROCESS                151
//#define _SC_SINGLE_PROCESS               152
//#define _SC_NETWORKING                   153
//#define _SC_READER_WRITER_LOCKS          154
//#define _SC_SPIN_LOCKS                   155
//#define _SC_REGEXP                       156
//#define _SC_REGEX_VERSION                157
//#define _SC_SHELL                        158
//#define _SC_SIGNALS                      159
//#define _SC_SPAWN                        160
//#define _SC_SPORADIC_SERVER              161
//#define _SC_THREAD_SPORADIC_SERVER       162
//#define _SC_SYSTEM_DATABASE              163
//#define _SC_SYSTEM_DATABASE_R            164
//#define _SC_TIMEOUTS                     165
//#define _SC_TYPED_MEMORY_OBJECTS         166
//#define _SC_USER_GROUPS                  167
//#define _SC_USER_GROUPS_R                168
//#define _SC_2_PBS                        169
//#define _SC_2_PBS_ACCOUNTING             170
//#define _SC_2_PBS_LOCATE                 171
//#define _SC_2_PBS_MESSAGE                172
//#define _SC_2_PBS_TRACK                  173
//#define _SC_SYMLOOP_MAX                  174
//#define _SC_STREAMS                      175
//#define _SC_2_PBS_CHECKPOINT             176
//#define _SC_V6_ILP32_OFF32               177
//#define _SC_V6_ILP32_OFFBIG              178
//#define _SC_V6_LP64_OFF64                179
//#define _SC_V6_LPBIG_OFFBIG              180
//#define _SC_HOST_NAME_MAX                181
//#define _SC_TRACE                        182
//#define _SC_TRACE_EVENT_FILTER           183
//#define _SC_TRACE_INHERIT                184
//#define _SC_TRACE_LOG                    185
//#define _SC_LEVEL1_ICACHE_SIZE           186
//#define _SC_LEVEL1_ICACHE_ASSOC          187
//#define _SC_LEVEL1_ICACHE_LINESIZE       188
//#define _SC_LEVEL1_DCACHE_SIZE           189
//#define _SC_LEVEL1_DCACHE_ASSOC          190
//#define _SC_LEVEL1_DCACHE_LINESIZE       191
//#define _SC_LEVEL2_CACHE_SIZE            192
//#define _SC_LEVEL2_CACHE_ASSOC           193
//#define _SC_LEVEL2_CACHE_LINESIZE        194
//#define _SC_LEVEL3_CACHE_SIZE            195
//#define _SC_LEVEL3_CACHE_ASSOC           196
//#define _SC_LEVEL3_CACHE_LINESIZE        197
//#define _SC_LEVEL4_CACHE_SIZE            198
//#define _SC_LEVEL4_CACHE_ASSOC           199
//#define _SC_LEVEL4_CACHE_LINESIZE        200
//#define _SC_IPV6                         201
//#define _SC_RAW_SOCKETS                  202
//#define _SC_V7_ILP32_OFF32               203
//#define _SC_V7_ILP32_OFFBIG              204
//#define _SC_V7_LP64_OFF64                205
//#define _SC_V7_LPBIG_OFFBIG              206
//#define _SC_SS_REPL_MAX                  207
//#define _SC_TRACE_EVENT_NAME_MAX         208
//#define _SC_TRACE_NAME_MAX               209
//#define _SC_TRACE_SYS_MAX                210
//#define _SC_TRACE_USER_EVENT_MAX         211
//#define _SC_XOPEN_STREAMS                212
//#define _SC_THREAD_ROBUST_PRIO_INHERIT   213
//#define _SC_THREAD_ROBUST_PRIO_PROTECT   214


#ifdef __KERNEL__
#include <kos/compiler.h>
__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Returns a negative kerrno_t on error, otherwise the value
extern __wunused long k_sysconf __P((int name));

__DECL_END
#elif !defined(__NO_PROTOTYPES)
#include <kos/syscall.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Returns a negative kerrno_t on error, otherwise the value
__local _syscall1(long,k_sysconf,int,name)

__DECL_END
#endif /* !__NO_PROTOTYPES */


#endif /* !__KOS_SYSCONF_H__ */
