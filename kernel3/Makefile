obj-m := fullkernel.o
fullkernel-objs := kernel3.o debug_nvdimm.o device_write.o device_read.o
//fullkernel-y    := kernel3.o debug_nvdimm.o device_write.o device_read.o 
ccflags-y := -std=gnu99 -Wno-declaration-after-statement	
COPTS=-g	

all:
	echo "Make target: all"
	make -C /lib/modules/$(shell uname -r)/build M=${shell pwd} modules
clean:
	echo "Make target: clean"	
	make -C /lib/modules/$(shell uname -r)/build M=${shell pwd} clean

build-tests:
	sudo dmesg -C
	sudo insmod kernel3.ko
	sudo lsmod | grep "kernel3"
	sudo dmesg | grep "URB"		
	sudo rmmod kernel3.ko
	sudo dmesg | grep "URB"	

test:
	sudo insmod kernel3.ko

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
	
	sudo rmmod kernel3.ko	