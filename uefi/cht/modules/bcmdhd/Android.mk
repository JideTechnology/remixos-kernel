ifeq (${COMBO_CHIP_VENDOR},bcm)
include $(call my-dir)/external_Android.mk

ifeq ($(GENERATE_INTEL_PREBUILTS),true)

.PHONY: bcmdhd_prebuilt

KMODULE_NAME_BCMDHD := bcmdhd
KMODULE_PREBUILT_BCMDHD := $(PRODUCT_OUT)/$(call intel-prebuilts-path,$(BCM43xx_PATH))
KMODULE_PREBUILD_SRC_BCMDHD := $(KMODULE_PREBUILT_BCMDHD)
ifneq ($(findstring 4324, $(COMBO_CHIP)), $(empty))
KMODULE_BIN_NAME_BCMDHD := bcmdhd
else
KMODULE_BIN_NAME_BCMDHD := bcmdhd_pcie
endif

$(KMODULE_PREBUILD_SRC_BCMDHD)/Makefile: $(BCM43xx_PATH)/Makefile.in
	$(hide) mkdir -p $(@D)
	$(hide) cp $< $@

bcmdhd_prebuilt: $(KMODULE_PREBUILD_SRC_BCMDHD)/Makefile
bcmdhd_prebuilt: bcmdhd_install | $(ACP)
	$(hide) echo Copying bcmdhd_bin.o as prebuilt
	$(hide) $(ACP) -fp $(PRODUCT_OUT)/linux/$(BCM43xx_PATH)/$(KMODULE_BIN_NAME_BCMDHD).o \
			   $(KMODULE_PREBUILD_SRC_BCMDHD)/$(KMODULE_BIN_NAME_BCMDHD)_bin.o_shipped
	$(hide) $(ACP) -fp $(BCM43xx_PATH)/external_Android.mk \
			   $(KMODULE_PREBUILT_BCMDHD)/Android.mk

# Make sure we install the prebuilt module for intel_prebuilts.
generate_intel_prebuilts: bcmdhd_prebuilt

endif # GENERATE_INTEL_PREBUILTS
endif # COMBO_CHIP_VENDOR
