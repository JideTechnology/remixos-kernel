# This makefile is included from vendor/intel/common/wifi/WifiRules.mk.
.PHONY: iwlwifi
INTEL_IWL_SRC_DIR := $(call my-dir)
INTEL_IWL_OUT_DIR := $(KERNEL_OUT_DIR)/../../$(INTEL_IWL_SRC_DIR)

iwlwifi: build_bzImage
	@echo Building kernel module iwlwifi in $(INTEL_IWL_OUT_DIR)
	@mkdir -p $(INTEL_IWL_OUT_DIR)
	@cp -rfl $(INTEL_IWL_SRC_DIR)/. $(INTEL_IWL_OUT_DIR)/
	@+$(KERNEL_BLD_ENV) $(MAKE) -C $(ANDROID_BUILD_TOP)/$(INTEL_IWL_OUT_DIR) ARCH=$(TARGET_ARCH) $(KERNEL_EXTRA_FLAGS) KLIB_BUILD=$(ANDROID_BUILD_TOP)/$(KERNEL_OUT_DIR) defconfig-xmm6321
	@+$(KERNEL_BLD_ENV) $(MAKE) -C $(ANDROID_BUILD_TOP)/$(INTEL_IWL_OUT_DIR) ARCH=$(TARGET_ARCH) $(KERNEL_EXTRA_FLAGS) KLIB_BUILD=$(ANDROID_BUILD_TOP)/$(KERNEL_OUT_DIR)

iwlwifi_clean:
	@echo Cleaning kernel module iwlwifi in $(INTEL_IWL_OUT_DIR)
	@rm -rf $(INTEL_IWL_OUT_DIR)

copy_modules_to_root: iwlwifi

ALL_KERNEL_MODULES += $(INTEL_IWL_OUT_DIR)

