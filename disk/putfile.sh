#!/bin/bash

src=$(readlink -f "$1")
dst=$(readlink -f "$2")
cd $(dirname $(readlink -f "$0"))

IMG='kos.img'

echo '../binutils/mtools-4.0.18/mcopy' -D o -i "$IMG" "$src" "::/$dst"
'../binutils/mtools-4.0.18/mcopy' -D o -i "$IMG" "$src" "::/$dst"
