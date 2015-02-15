KERNEL_DIR ?= $(LICHEE_KDIR)
BUILD_TYPE ?= debug
include $(KERNEL_DIR)/.config

##############################################################
##################     Set SYSTEM_TYPE      ##################
##############################################################
ifeq ($(SYSTEM_TYPE),)
SYSTEM_TYPE ?= $(LICHEE_PLATFORM)
endif

##############################################################
##################   Set ARCH and CROSS_COMPILE  #############
##############################################################
ifdef CONFIG_ARM
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabi-
endif
	
ifdef CONFIG_ARM64
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
endif

##############################################################
##################     Set GPU_TYPE         ##################
##############################################################
ifeq ($(GPU_TYPE),)
#####################################
#For mali400 Platform
#####################################
ifdef CONFIG_ARCH_SUN8IW3P1
GPU_TYPE = mali400
endif

ifdef CONFIG_ARCH_SUN8IW5P1
GPU_TYPE = mali400
endif

ifdef CONFIG_ARCH_SUN8IW7P1
GPU_TYPE = mali400
endif

ifdef CONFIG_ARCH_SUN50IW1P1
GPU_TYPE = mali400
endif

#####################################
#For sgx544 Platform
#####################################
ifdef CONFIG_ARCH_SUN8IW6P1
GPU_TYPE = sgx544
endif

#####################################
#For rgx6230 Platform
#####################################
ifdef CONFIG_ARCH_SUN9IW1P1
GPU_TYPE = rgx6230
endif

endif
##############################################################
#############  Set GPU_BUILD_DIR and GPU_KO_NAME ##############
##############################################################
#####################################
#For mali400 Platform
#####################################
ifeq ($(GPU_TYPE),mali400)
GPU_BUILD_DIR = mali400/driver/src/devicedrv/mali/
GPU_KO_NAME   = $(GPU_BUILD_DIR)/mali.ko
ifdef CONFIG_OF
export USING_DT = 1
endif
endif
##############################################################

all: build install

build:
	$(MAKE) -j -C $(GPU_BUILD_DIR) BUILD_TYPE=$(BUILD_TYPE) KDIR=$(KERNEL_DIR) ARCH=$(ARCH) USING_UMP=0
	# $(MAKE) -j ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KDIR=$(LICHEE_KDIR) USING_DT=1 USING_UMP=0 BUILD=debug
	
install:
	if [ -n $(LICHEE_MOD_DIR) ]; then \
		cp -u $(GPU_KO_NAME) $(LICHEE_MOD_DIR); \
	fi

clean:
	$(MAKE) -C $(GPU_BUILD_DIR) clean
	rm -rf $(GPU_CLEAR_DIR)
	rm -rf $(GPU_KO_NAME)

