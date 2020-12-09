# https://stackoverflow.com/questions/3467850/cross-compiling-a-kernel-module
ARCH=arm
COMPILER=arm-linux-gnueabihf-

ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib

EXTRA_CFLAGS = -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/

obj-m := ebbchar.o
KERNELDIR := /home/labarqcomp/Desktop/embarcados-deco/linux
PWD := $(shell pwd)

IP = 192.168.1.161
DEST = /root/driver/
PASS = 1234

all: ebbchar.ko, test

ebbchar.ko: ebbchar.c
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) modules

test:
	$(COMPILER)gcc testebbchar.c -o test

deploy: ebbchar.ko test
	sshpass -p $(PASS) scp test root@$(IP):$(DEST)
	sshpass -p $(PASS) scp ebbchar.ko root@$(IP):$(DEST)

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) clean
	rm test

