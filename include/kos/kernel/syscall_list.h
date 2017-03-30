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

#ifndef __DEEMON__
#ifndef SYSCALL
#error "Must #define SYSCALL before #including this file"
#endif
#ifndef SYSCALL_NULL
#error "Must #define SYSCALL_NULL before #including this file"
#endif
#endif

/*[[[deemon
#include <file>
#include <util>
#include <fs>

fs::chdir(fs::path::head(__FILE__));
local syscalls = [];
for (local l: file.open("../syscallno.h","r")) {
  local name,id;
  try {
    name,id = l.scanf(" # define SYS_%[^ ] %[0-9A-Fa-fxX] ")...;
    id = (int)id;
  } catch (...) continue;
  if (id >= #syscalls) syscalls.resize(id+1,none);
  if (syscalls[id] !is none) {
    print file.io.stderr: "SYSCALL",name," ("+id+") is already used by",syscalls[id];
  }
  syscalls[id] = name;
}
function print_id(i) {
  print ("/" "* %6d|%#.4x *" "/" % (i,i)),;
}

for (local i,n: util::enumerate(syscalls)) {
  print_id(i);
  if (n is none) {
    print " SYSCALL_NULL";
  } else {
    print " SYSCALL("+n+")";
  }
}
local total_count = #syscalls;
local padding = 1,idbits = 1;
while (padding < total_count) padding *= 2,++idbits;
print "#undef SYSCALL_COUNT";
print "#define SYSCALL_COUNT "+total_count;
print "#ifdef SYSCALL_PAD";
print "#undef SYSCALL_TOTAL";
print "#undef SYSCALL_IDBITS";
print "#define SYSCALL_TOTAL  "+padding;
print "#define SYSCALL_IDBITS "+(idbits-1);
padding -= total_count;
for (local i: util::range(padding)) {
  print_id(total_count+i);
  print " SYSCALL_PAD";
}
print "#endif /" "* SYSCALL_PAD *" "/";
]]]*/
/*      0|0x0000 */ SYSCALL(k_syslog)
/*      1|0x0001 */ SYSCALL(k_sysconf)
/*      2|0x0002 */ SYSCALL(kfd_open)
/*      3|0x0003 */ SYSCALL(kfd_open2)
/*      4|0x0004 */ SYSCALL(kfd_close)
/*      5|0x0005 */ SYSCALL(kfd_closeall)
/*      6|0x0006 */ SYSCALL(kfd_seek)
/*      7|0x0007 */ SYSCALL(kfd_read)
/*      8|0x0008 */ SYSCALL(kfd_write)
/*      9|0x0009 */ SYSCALL(kfd_pread)
/*     10|0x000a */ SYSCALL(kfd_pwrite)
/*     11|0x000b */ SYSCALL(kfd_flush)
/*     12|0x000c */ SYSCALL(kfd_trunc)
/*     13|0x000d */ SYSCALL(kfd_fcntl)
/*     14|0x000e */ SYSCALL(kfd_ioctl)
/*     15|0x000f */ SYSCALL(kfd_getattr)
/*     16|0x0010 */ SYSCALL(kfd_setattr)
/*     17|0x0011 */ SYSCALL(kfd_readdir)
/*     18|0x0012 */ SYSCALL(kfd_readlink)
/*     19|0x0013 */ SYSCALL(kfd_dup)
/*     20|0x0014 */ SYSCALL(kfd_dup2)
/*     21|0x0015 */ SYSCALL(kfd_pipe)
/*     22|0x0016 */ SYSCALL(kfd_equals)
/*     23|0x0017 */ SYSCALL(kfd_openpty)
/*     24|0x0018 */ SYSCALL(kfs_mkdir)
/*     25|0x0019 */ SYSCALL(kfs_rmdir)
/*     26|0x001a */ SYSCALL(kfs_unlink)
/*     27|0x001b */ SYSCALL(kfs_remove)
/*     28|0x001c */ SYSCALL(kfs_symlink)
/*     29|0x001d */ SYSCALL(kfs_hrdlink)
/*     30|0x001e */ SYSCALL(ktime_getnow)
/*     31|0x001f */ SYSCALL(ktime_setnow)
/*     32|0x0020 */ SYSCALL(ktime_htick)
/*     33|0x0021 */ SYSCALL(kmem_map)
/*     34|0x0022 */ SYSCALL(kmem_unmap)
/*     35|0x0023 */ SYSCALL(kmem_validate)
/*     36|0x0024 */ SYSCALL(kmem_mapdev)
/*     37|0x0025 */ SYSCALL(ktask_yield)
/*     38|0x0026 */ SYSCALL(ktask_setalarm)
/*     39|0x0027 */ SYSCALL(ktask_getalarm)
/*     40|0x0028 */ SYSCALL(ktask_abssleep)
/*     41|0x0029 */ SYSCALL(ktask_terminate)
/*     42|0x002a */ SYSCALL(ktask_suspend)
/*     43|0x002b */ SYSCALL(ktask_resume)
/*     44|0x002c */ SYSCALL(ktask_openroot)
/*     45|0x002d */ SYSCALL(ktask_openparent)
/*     46|0x002e */ SYSCALL(ktask_openproc)
/*     47|0x002f */ SYSCALL(ktask_getparid)
/*     48|0x0030 */ SYSCALL(ktask_gettid)
/*     49|0x0031 */ SYSCALL(ktask_openchild)
/*     50|0x0032 */ SYSCALL(ktask_enumchildren)
/*     51|0x0033 */ SYSCALL(ktask_getpriority)
/*     52|0x0034 */ SYSCALL(ktask_setpriority)
/*     53|0x0035 */ SYSCALL(ktask_join)
/*     54|0x0036 */ SYSCALL(ktask_tryjoin)
/*     55|0x0037 */ SYSCALL(ktask_timedjoin)
/*     56|0x0038 */ SYSCALL(ktask_newthread)
/*     57|0x0039 */ SYSCALL(ktask_newthreadi)
/*     58|0x003a */ SYSCALL(ktask_fork)
/*     59|0x003b */ SYSCALL(ktask_exec)
/*     60|0x003c */ SYSCALL(ktask_fexec)
/*     61|0x003d */ SYSCALL(kfutex_cmd)
/*     62|0x003e */ SYSCALL(kfutex_ccmd)
/*     63|0x003f */ SYSCALL_NULL
/*     64|0x0040 */ SYSCALL_NULL
/*     65|0x0041 */ SYSCALL(kproc_enumfd)
/*     66|0x0042 */ SYSCALL(kproc_openfd)
/*     67|0x0043 */ SYSCALL(kproc_openfd2)
/*     68|0x0044 */ SYSCALL(kproc_barrier)
/*     69|0x0045 */ SYSCALL(kproc_openbarrier)
/*     70|0x0046 */ SYSCALL(kproc_tlsalloc)
/*     71|0x0047 */ SYSCALL(kproc_tlsfree)
/*     72|0x0048 */ SYSCALL(kproc_tlsenum)
/*     73|0x0049 */ SYSCALL(kproc_enumpid)
/*     74|0x004a */ SYSCALL(kproc_openpid)
/*     75|0x004b */ SYSCALL(kproc_getpid)
/*     76|0x004c */ SYSCALL(kproc_getenv)
/*     77|0x004d */ SYSCALL(kproc_setenv)
/*     78|0x004e */ SYSCALL(kproc_delenv)
/*     79|0x004f */ SYSCALL(kproc_enumenv)
/*     80|0x0050 */ SYSCALL(kproc_getcmd)
/*     81|0x0051 */ SYSCALL(kproc_perm)
/*     82|0x0052 */ SYSCALL(kmod_open)
/*     83|0x0053 */ SYSCALL(kmod_fopen)
/*     84|0x0054 */ SYSCALL(kmod_close)
/*     85|0x0055 */ SYSCALL(kmod_sym)
/*     86|0x0056 */ SYSCALL(kmod_info)
/*     87|0x0057 */ SYSCALL(kmod_syminfo)
/*     88|0x0058 */ SYSCALL(ktime_hfreq)
#undef SYSCALL_COUNT
#define SYSCALL_COUNT 89
#ifdef SYSCALL_PAD
#undef SYSCALL_TOTAL
#undef SYSCALL_IDBITS
#define SYSCALL_TOTAL  128
#define SYSCALL_IDBITS 7
/*     89|0x0059 */ SYSCALL_PAD
/*     90|0x005a */ SYSCALL_PAD
/*     91|0x005b */ SYSCALL_PAD
/*     92|0x005c */ SYSCALL_PAD
/*     93|0x005d */ SYSCALL_PAD
/*     94|0x005e */ SYSCALL_PAD
/*     95|0x005f */ SYSCALL_PAD
/*     96|0x0060 */ SYSCALL_PAD
/*     97|0x0061 */ SYSCALL_PAD
/*     98|0x0062 */ SYSCALL_PAD
/*     99|0x0063 */ SYSCALL_PAD
/*    100|0x0064 */ SYSCALL_PAD
/*    101|0x0065 */ SYSCALL_PAD
/*    102|0x0066 */ SYSCALL_PAD
/*    103|0x0067 */ SYSCALL_PAD
/*    104|0x0068 */ SYSCALL_PAD
/*    105|0x0069 */ SYSCALL_PAD
/*    106|0x006a */ SYSCALL_PAD
/*    107|0x006b */ SYSCALL_PAD
/*    108|0x006c */ SYSCALL_PAD
/*    109|0x006d */ SYSCALL_PAD
/*    110|0x006e */ SYSCALL_PAD
/*    111|0x006f */ SYSCALL_PAD
/*    112|0x0070 */ SYSCALL_PAD
/*    113|0x0071 */ SYSCALL_PAD
/*    114|0x0072 */ SYSCALL_PAD
/*    115|0x0073 */ SYSCALL_PAD
/*    116|0x0074 */ SYSCALL_PAD
/*    117|0x0075 */ SYSCALL_PAD
/*    118|0x0076 */ SYSCALL_PAD
/*    119|0x0077 */ SYSCALL_PAD
/*    120|0x0078 */ SYSCALL_PAD
/*    121|0x0079 */ SYSCALL_PAD
/*    122|0x007a */ SYSCALL_PAD
/*    123|0x007b */ SYSCALL_PAD
/*    124|0x007c */ SYSCALL_PAD
/*    125|0x007d */ SYSCALL_PAD
/*    126|0x007e */ SYSCALL_PAD
/*    127|0x007f */ SYSCALL_PAD
#endif /* SYSCALL_PAD */
//[[[end]]]
