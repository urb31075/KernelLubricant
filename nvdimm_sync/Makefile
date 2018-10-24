KDIR = /lib/modules/$(shell uname -r)/build

obj-m := nvdimm_alloc.o

default: clean
	make -C $(KDIR) M=$(shell pwd) modules

debug: MY_CFLAGS=-DDEBUG
debug: clean
	make -C $(KDIR) M=$(shell pwd) EXTRA_CFLAGS="$(MY_CFLAGS)" modules
	
clean:
	rm -rf *.ko *.cmd *.o .driver.* Module.symvers *.mod.c .tmp_versions *.order
