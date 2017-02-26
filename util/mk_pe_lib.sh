#!/bin/bash

#
# Really hacky way of doing the impossible:
#  >> Generate both a .def and .lib files from a .so
#
# This is a required process to hack gcc into linking a
# PE .exe against what is actually an ELF .so file,
# allowing the KOS linker to actually cross-link
# between the windows and the linux world!
#
# Clarification:
#  .exe : Windows PE binary (linux equivalent has no extension)
#  .so  : Linux ELF shared library (Windows equivalent is a .dll)
#  .dll : Windows PE shared library (Linux equivalent is a .so)
#  .def : A text file containing all exports of a library
#  .lib : Linker definitions required by windows gcc ports to link an exe against a dll.
#
# This script breaks the rules and allows the KOS linker to function as
# intended by creating a .lib file from a .so, instead of an intended .dll.
#

ROOT=$(dirname $(readlink -f "$0"))
APP_NM="$ROOT/../binutils/build-binutils-i686-elf/bin/i686-elf-nm"
APP_DLLTOOL="/cygdrive/d/cygwin32/bin/dlltool"

do_file() {
	shfile="$1"
	shpath=$(dirname "$shfile")
	shfullname=$(basename "$shfile")
	shname="${shfullname%.*}"
	deffile="$shpath/$shname.def"
	libfile="$shpath/$shname.lib"
	echo "Generating .def file $deffile for $shfile..."
	echo "EXPORTS" > "$deffile"
	"$APP_NM" -g "$shfile" | grep ' T ' | sed 's/.* T //' >> "$deffile"
	echo "Generating .lib file $libfile for $shfile..."
	"$APP_DLLTOOL" --def "$deffile" --dllname "$shfullname" --output-lib "$libfile"
}

do_file "$1"

# i686-pc-cygwin-gcc.exe -nostdlib -o bin\userland\pe-test -Iinclude -Lbin\libs userland\pe-test.c bin\libs\libc.lib
