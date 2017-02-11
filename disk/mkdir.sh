#!/bin/bash

path=$(readlink -f "$1")

cd $(dirname $(readlink -f "$0"))
IMG='kos.img'
'../binutils/mtools-4.0.18/mmd' -D o -i "$IMG" "::/$path"
