
The hard disk image can be created with:

$ qemu-img create -f qcow2 kos.img 1G
$ ../binutils/mtools-4.0.18/mformat -t 80 -h 2 -s 18 -i kos.img
$ ../binutils/mtools-4.0.18/mcopy -i kos.img README.txt ::/
