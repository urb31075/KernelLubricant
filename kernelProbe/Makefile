obj-m += kernelProbe.o 

all:
	echo "Make target: all"
	make -C /lib/modules/$(shell uname -r)/build M=${shell pwd} modules -Wno-unused-function
clean:
	echo "Make target: clean"	
	make -C /lib/modules/$(shell uname -r)/build M=${shell pwd} clean

build-tests:
	sudo dmesg -C
	-sudo insmod kernelProbe.ko
test:	
	sudo dmesg
