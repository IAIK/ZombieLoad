LKM             = devmem_mod

obj-m          += $(LKM).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

unload:
	sudo rmmod $(LKM).ko || true

load: unload all
	sudo modprobe -a msr
	sudo insmod $(LKM).ko
	dmesg | tail
