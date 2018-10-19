obj-m += kernel2.o

all:
	echo "Make target: all"
	make -C /lib/modules/$(shell uname -r)/build M=${shell pwd} modules
clean:
	echo "Make target: clean"	
	make -C /lib/modules/$(shell uname -r)/build M=${shell pwd} clean

build-tests:
	sudo dmesg -C
	sudo insmod kernel2.ko
	sudo lsmod | grep "kernel2"
	sudo dmesg | grep "URB"		
	sudo rmmod kernel2.ko
	sudo dmesg | grep "URB"	

test:
	sudo insmod kernel2.ko

	- /tmp/kernelunittest1
	- /tmp/kernelunittest2
	- /tmp/kernelunittest3
	- /tmp/kernelunittest4	
	- /tmp/kernelunittest1
	- /tmp/kernelunittest2
	- /tmp/kernelunittest3
	- /tmp/kernelunittest4		
	- /tmp/kernelunittest1
	- /tmp/kernelunittest2
	- /tmp/kernelunittest3
	- /tmp/kernelunittest4	
	- /tmp/kernelunittest1
	- /tmp/kernelunittest2
	- /tmp/kernelunittest3
	- /tmp/kernelunittest4	
	
	sudo rmmod kernel2.ko	