# SIMPLE MAKEFILE KERNEL - SKY
# v2

# FORCE VERSION
#VER	?= 2.6.38.2-386

VER	?= $(shell uname -r)
obj-m	:= arcade.o
TARGET	:= arcade
MODNAME	:= arcade.ko

KDIR	:= /lib/modules/$(VER)/build
PWD	:= $(shell pwd)

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD)  modules

clean:
	$(MAKE) -C /lib/modules/$(VER)/build M=$(PWD) clean

modules_install: $(MODNAME)
	mkdir -p /lib/modules/$(VER)/kernel/misc
	install -m 0644 -c $(TARGET).ko /lib/modules/$(VER)/kernel/misc ||      install -m 0644 -c $(TARGET).o /lib/modules/$(VER)/kernel/misc
	depmod -a $(VER)

