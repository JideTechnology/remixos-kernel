#
# Makefile for sunxi system information driver
#

PWD=$(shell pwd)

obj-$(CONFIG_SUNXI_SYS_INFO) += sunxi_sys_info.o chipid-sunxi.o
obj-$(CONFIG_ARCH_SUN50IW1P1) += smc_call.o

install: build
	cp sunxi_sys_info.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

#clean:
#	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
