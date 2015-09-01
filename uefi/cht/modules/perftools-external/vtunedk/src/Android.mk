LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := sep3_15
LOCAL_MODULE_PATH := $(LOCAL_PATH)
LOCAL_MODULE_TAGS := debug
ifneq ($(TARGET_BUILD_VARIANT),user)
$(eval $(call build_kernel_module,$(LOCAL_PATH),vtunedk))
endif
