#MAKEFILE KERNEL SKY
# v2
# TODO: distinguir si kernel < 2.6.15, para usar arcade-old.c
#       distinguir si kernel > 2.6.17, para usar arcade-new.c

# FORZAR VERSION
#VER	?= 2.6.15-28-386

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

