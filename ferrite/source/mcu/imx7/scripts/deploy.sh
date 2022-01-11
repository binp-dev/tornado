#!/bin/sh

cat ./build/release/m4image.bin | \
ssh root@$1 "cat > m4image.bin && mount /dev/mmcblk0p1 /mnt && mv m4image.bin /mnt && umount /mnt"

if [ "$2" = "--reboot" ]; then
    ssh root@$1 "reboot now"
fi
