#!/bin/bash
cd $(dirname `readlink -f "$0"`)

IMG='kos.img'

mcopy()    { '../binutils/mtools-4.0.18/mcopy' $*; }
mdel()     { '../binutils/mtools-4.0.18/mdel' $*; }
mdir()     { '../binutils/mtools-4.0.18/mdir' $*; }
mmd()      { '../binutils/mtools-4.0.18/mmd' $*; }
mformat()  { '../binutils/mtools-4.0.18/mformat' $*; }
qemu-img() { '/cygdrive/d/qemu/qemu-img.exe' $*; }

md() { mmd   -D s -i "$IMG"      "::/$1"; }
pf() { mcopy -D o -i "$IMG" "$1" "::/$2"; }

if ! [ -f "$IMG" ]; then
	qemu-img create -f qcow2 "$IMG" 1G > /dev/null
	mformat -t 8 -h 2 -s 360 -i "$IMG"
fi

md                 /etc
md                 /usr
pf hdd/etc/passwd  /etc/passwd
pf bigfile.h       /bigfile.h

# disk_put readme.txt
# disk_mkdir /bin
# disk_put ../bin/userland/ls /bin/ls


