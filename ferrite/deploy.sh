#!/bin/sh

cat ./build/release/m4image.bin | ssh root@$1 "cat > m4image.bin && mount /dev/mmcblk0p1 /mnt && cp m4image.bin /mnt && umount /mnt && rm m4image.bin"
