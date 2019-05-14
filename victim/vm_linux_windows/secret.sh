#!/bin/sh
qemu-system-x86_64 -kernel bzImage -drive file=rootfs.ext2,if=virtio,format=raw -append 'root=/dev/vda nopti console=ttyS0' -net nic,model=virtio -net user -enable-kvm -serial stdio 
