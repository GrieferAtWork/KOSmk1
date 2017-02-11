#!/bin/bash

# Fix $PATH to include the right GCC
KOS_ROOT=$(dirname $(readlink -f "$0"))
export PREFIX="$KOS_ROOT/binutils/build-binutils-i686-elf"
export PATH="$PREFIX/bin:$PATH"

export HOST="i386-kos"

# Setup a compiler environment
alias _ld="i686-elf-ld"
alias _gcc="i686-elf-gcc"
alias _cpp="i686-elf-cpp"
alias _g++="i686-elf-g++"

KOS_INCLUDE="$KOS_INCLUDE -I $KOS_ROOT/include"
KOS_DEFINE="$KOS_DEFINE -D__i386__=1"
KOS_DEFINE="$KOS_DEFINE -D_FILE_OFFSET_BITS=1"
KOS_LIBPATH="$KOS_LIBPATH -L $KOS_ROOT/bin/libs"
KOS_LIBPATH="$KOS_LIBPATH -L $KOS_ROOT/binutils/build-gcc-i686-elf/i686-elf/libgcc"
KOS_LIBS="$KOS_LIBS -l_start"
KOS_LIBS="$KOS_LIBS -lc"
KOS_LIBS="$KOS_LIBS -lgcc"

gcc() { _gcc -static-libgcc -nostdlib $KOS_DEFINE $KOS_INCLUDE $KOS_LIBPATH $* $KOS_LIBS; }
ld()  { _ld  -static-libgcc -nostdlib $KOS_LIBPATH $* $KOS_LIBS; }
cpp() { _cpp -nostdlib $KOS_DEFINE $KOS_INCLUDE $*; }
g++() { _g++ -static-libgcc -nostdlib $KOS_DEFINE $KOS_INCLUDE $KOS_LIBPATH $* $KOS_LIBS; }


alias addr2line="i686-elf-addr2line"
alias ar="i686-elf-ar"
alias as="i686-elf-as"
alias c++="i686-elf-c++"
alias c++filt="i686-elf-c++filt"
alias elfedit="i686-elf-elfedit"
alias gcc-6.2.0="gcc"
alias gcc-ar="i686-elf-gcc-ar"
alias gcc-nm="i686-elf-gcc-nm"
alias gcc-ranlib="i686-elf-gcc-ranlib"
alias gcov="i686-elf-gcov"
alias gcov-tool="i686-elf-gcov-tool"
alias gprof="i686-elf-gprof"
alias ld.bfd="i686-elf-ld.bfd"
alias nm="i686-elf-nm"
alias objcopy="i686-elf-objcopy"
alias objdump="i686-elf-objdump"
alias ranlib="i686-elf-ranlib"
alias readelf="i686-elf-readelf"
alias size="i686-elf-size"
alias strings="i686-elf-strings"
alias strip="i686-elf-strip"









