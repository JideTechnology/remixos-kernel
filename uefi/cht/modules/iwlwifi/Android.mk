# including iwlwifi Android mk only if it was declared.
ifeq ($(BOARD_USING_INTEL_IWL),true)

LOCAL_PATH := $(call my-dir)

# Sofia projects

ifeq ($(INTEL_IWL_USE_SYSTEM_COMPAT_MOD_BUILD),y)
# Build iwlwifi modules using system compat install

include $(CLEAR_VARS)

LOCAL_MODULE := IWLWIFI
LOCAL_KERNEL_COMPAT_DEFCONFIG := $(INTEL_IWL_BOARD_CONFIG)

# $1: INSTALL_MOD_PATH
# $2: Module source dir
define COMPAT_PRIVATE_$(LOCAL_MODULE)_PREINSTALL
        find $(1) -name modules.dep -exec cp {} $(2)/modules.dep.orig \;
endef

# $1: INSTALL_MOD_PATH
# $2: Module source dir
define COMPAT_PRIVATE_$(LOCAL_MODULE)_POSTINSTALL
        find $(1) -path \*updates\*\.ko -type f -exec $(2)/intc-scripts/mv-compat-mod.py {} iwlmvm \;
        find $(1) -name modules.dep -exec $(2)/intc-scripts/ren-compat-deps.py {} updates iwlmvm \;
        find $(1) -name modules.dep -exec sh -c 'cat $(2)/modules.dep.orig >> {}' \;
        find $(1) -name modules.alias -exec $(2)/intc-scripts/ren-compat-aliases.py {} iwlwifi \;
endef

include $(BUILD_COMPAT_MODULE)

else

# CHT51
#
# include only the rigth Android-make-iwlwifi.mk file.
#
# in the CHT51 tree we may have multiple iwlwifi in different folders, this will cause
# that the only one that will be evaluated is the last one included.
#
# the preferred way to handle this is to include the right one from the BoardConfig.mk file,
# but looks like some of the needed variables are not defined when this file is included, what will
# cause a failure in the build. so we need to include the file later on, when all Android mk are
# included and all variables are defined. so we do this from here.
#
# (for now) INTEL_IWL_MODULE_SUB_FOLDER can hold uefi/cht , sofia/lte , sofia/3gr

ifeq ($(INTEL_IWL_MODULE_SUB_FOLDER),$(LOCAL_PATH))
	include $(INTEL_IWL_MODULE_SUB_FOLDER)/Android-make-iwlwifi.mk
endif

endif # ifeq ($(INTEL_IWL_USE_SYSTEM_COMPAT_MOD_BUILD),y)

endif

