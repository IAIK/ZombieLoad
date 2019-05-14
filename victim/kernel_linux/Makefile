obj-m += leaky.o
ccflags-y += -Wno-unused-result
all: leaky.c test.c
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -O3 test.c -o secret
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f secret
	

