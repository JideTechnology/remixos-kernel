#
# Version: 1.0
#
# Copyright (C) 2009-2015 Intel Corporation.  All Rights Reserved.
#
#

# -------------------- user configurable options ------------------------

# base name of driver
DRIVER_NAME = lm_dump

# location to install driver
INSTALL = .

# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
EXTRA_CFLAGS += -DDRV_ANDROID

# if ARCH variable is set, unset it to avoid conflicts with kbuild
unexport ARCH

# platform details
MACH ?= i686
ifeq ($(MACH),x86_64)
PLATFORM=x32_64
endif
ifeq ($(PLATFORM),)
PLATFORM=x32
endif

# eventual filename of the driver
DRIVER_FILENAME=$(DRIVER_NAME).ko

# build options ...
ifneq ($(KERNELRELEASE),)
	obj-m := $(DRIVER_NAME).o

# targets ...

# Otherwise, we were called directly from the command
# line; invoke the kernel build system.
else
        PWD := $(shell pwd)
        KERNEL_SRC_DIR := $(PWD)/../../../../$(KERNEL_SRC_DIR)

all: default

default:
	$(MAKE) -C $(KERNEL_SRC_DIR) M=$(PWD) modules PWD=$(PWD)

endif

install:
	@cp $(DRIVER_NAME).ko $(INSTALL)/$(DRIVER_FILENAME)
	@echo "Installed $(DRIVER_NAME) driver to $(INSTALL)/$(DRIVER_FILENAME) ."

clean:
	@rm -f *.o .*.o.cmd .*.o.d .*.ko.cmd .*.ko.unsigned.cmd *.gcno
	@rm -f $(DRIVER_NAME).ko $(DRIVER_NAME).ko.unsigned $(DRIVER_FILENAME)
	@rm -f Module.symvers Modules.symvers modules.order Module.markers
	@rm -rf .tmp_versions

distclean: clean
	@rm -f $(DRIVER_NAME)*.ko
