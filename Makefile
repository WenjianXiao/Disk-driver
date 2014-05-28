#Disable/enable debugging
DEBUG = y

#Add debugging flag (or not) to EXTRA_CFLAGS
ifeq ($(DEBUG), y)
	DEBFLAGS = -O -g
else
	DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)

#Call from kernel build system if KERNELREALSE defined
ifneq ($(KERNELRELEASE),)
	obj-m :=module.o
	module-objs := xlg-dev.o xcache-dev.o xph-dev.o

#Otherwise call from command line
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif


clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
