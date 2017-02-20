
### <b>KOS (Hooby) Operating System & Kernel</b> ###

Like others of its kind, KOS (<i>pronounced chaos</i>) is a home-made operating system build around the idea of sandboxing.

Following a strict permission tree while implementing as many Posix standards as possible, KOS is designed around the idea that a process inherits its parents permissions upon creation and is only capable of taking away its own permissions:

Posix: <b>fork()</b>+<b>exec()</b><br>
KOS:   <b>fork()</b>+[<b>restrict_permissions()</b>...]+<b>exec()</b>

Besides being Posix-oriented, KOS also brings some interesting new things to the table, such as (what I call) signal-based synchronization, and the expansion of file descriptor functionality.

KOS is written with multitasking and the ability of spuriously terminating any process in mind, as well as constant consideration of security weighed against cross-task control meant to allow user application to regulate their own permissions with the end goal of natively running early windows programs side-by-side with POSIX applications without.

Written from the ground up, KOS has its own version of libc, libdl and others, bringing a lot of new functionality to the table in terms of <i>missing</i> stdc-functions such as 'strdupf' or 'strend', as well as an integrated memory leak detector and its own implementation of curses (NOTE: It's no longer debug_new. - I've learned a lot since then...).

While KOS is not meant to re-invent the wheel, making use of proprietary libraries such as dlmalloc and fdlibm, it is a learning experience that has already tought me a lot about the X86 architecture as well as assembly; the simplest, yet also hardest language there is.

As usual, this is a <b>one-person project</b>, with development having started on <b>30.11.2016</b>.

Chaos|KOS - Total anarchy.

## Working ##
 - i386 (Correct types are always used; x86-64 support is planned)
 - QEMU (Real hardware, or other emulators have not been tested (yet))
 - multiboot (GRUB)
 - dynamic memory (malloc)
 - syscall
 - multitasking/scheduler
   - pid/tid
   - low-cpu/true idle
   - process
     - argc|argv
     - environ
     - fork
     - exec
     - pipe (Missing named pipes)
     - sandbox-oriented security model (barriers)
     - shared memory (Missing from user-space)
       - Reference-counted memory
       - Full copy-on-write support
     - mmap/munmap/brk/sbrk
   - threads
     - new_thread
     - terminate
     - suspend/resume
     - priorities
     - names
     - detach/join
     - yield
     - alarm/pause
     - tls
   - synchronization primitives (Missing from user-space)
     - semaphore
     - mutex
     - rwlock
     - spinlock
     - mmutex (non-standard primitive)
     - signal+vsignal (non-standard primitive)
     - addist+ddist (non-standard primitive)
 - ELF binaries
 - dlopen (Shared libraries)
 - Time (time_t/struct timespec)
 - Ring #3
 - Filesystem
   - FAT-12/16/32
   - open/read/write/seek/etc.
   - OK:      Read/write existing files
   - MISSING: create/delete/rename files/folders
   - VFS (Virtual filesystem)
     - OK:      /dev, /proc (incomplete)
     - MISSING: /sys
 - Highly posix-compliant
 - PS/2 keyboard input
 - ANSI-compliant Terminal
 - User-space commandline (/bin/sh)





### Build Requirements ###
 - GCC cross-compiler build for i386 (No special modifications needed)
   - step-by-step instructions here: http://wiki.osdev.org/GCC_Cross-Compiler
   - NOTE: I'm using GCC 6.2.0, but older and newer versions should work, too...
 - deemon (https://github.com/GrieferAtWork/deemon)
   - If the latest commit doesn't work, use this one:<br>
     https://github.com/GrieferAtWork/deemon/commit/f03dab36d915205a21347ac7c4855d2fa73f9e3a
 - QEMU

Yes, KOS doesn't have what would technically be considered a toolchain. Instead, everything is done through magic(.dee)

## Setup ##
 - Potentially need to edit '/magic.dee' to use correct paths for compilers
 - Though kind-of strange, I use Visual Studio as IDE and windows as host platform, with it being possible that compilation in another environment than my (admittedly _very_ strange) one may require additional steps to be taken.

## Build + Update + Link + Run ##
:/$ deemon magic.dee




