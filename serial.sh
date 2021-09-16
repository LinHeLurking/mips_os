#!/bin/bash

if [ -f "disk" ];then
    echo -e "disk exsits, creating image and burning it into it\n"
else
    echo -e "disk does not exist, creating one\n"
    dd if=/dev/zero of=disk bs=4096 count=1M
fi



if [[ ! "$TERM" =~ "screen".* ]];then
    echo "Use this script in tmux!"
else
    echo "Writing image into disk"

    dd if=image of=disk conv=notrunc

    tmux split-window -h 'gdb-multiarch -x ../init.gdb'

#    SERIAL=2 ../QEMULoongson/qemu/bin/qemu-system-mipsel -M ls1c -vnc 127.0.0.1:0 -kernel ../QEMULoongson/bios/gzram -gdb tcp::50010 -m 32 -usb -drive file=disk,id=a,if=none -device usb-storage,bus=usb-bus.1,drive=a -net tap,ifname=tap0,script=no,downscript=no -net nic -serial stdio  "$@" -k /usr/share/qemu/keymaps/en-us
    SERIAL=2 ../QEMULoongson/qemu/bin/qemu-system-mipsel -M ls1c -vnc 127.0.0.1:0 -kernel ../QEMULoongson/bios/gzram -gdb tcp::50010 -m 32 -usb -drive file=disk,id=a,if=none -device usb-storage,bus=usb-bus.1,drive=a -serial stdio  "$@" -k /usr/share/qemu/keymaps/en-us
    clear
fi
