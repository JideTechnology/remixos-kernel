# parts of build/core/tasks/kernel.mk

WL_ENABLED := $(if $(wildcard $(WL_PATH)),$(shell grep ^CONFIG_WL=[my] $(KERNEL_CONFIG_FILE)))
WL_ARCH_CHANGED := $(if $(shell file $(WL_PATH)/lib/wlc_hybrid.o_shipped | grep -s $(if $(filter x86,$(TARGET_KERNEL_ARCH)),80386,x86-64)),,FORCE)
WL_SRC := $(WL_PATH)/hybrid-v35$(if $(filter x86,$(TARGET_KERNEL_ARCH)),,_64)-nodebug-pcoem-6_30_223_271.tar.gz
$(WL_SRC):
	@echo Downloading $(@F)...
	$(hide) curl http://www.broadcom.com/docs/linux_sta/$(@F) > $@

$(WL_PATH)/Makefile : $(WL_SRC) $(wildcard $(WL_PATH)/*.patch) $(WL_ARCH_CHANGED) $(KERNEL_ARCH_CHANGED)
	$(hide) tar zxf $< -C $(@D) --overwrite && \
		patch -p5 -d $(@D) -i wl.patch && \
		patch -p1 -d $(@D) -i linux-recent.patch

$(INSTALLED_KERNEL_TARGET): $(if $(WL_ENABLED),$(WL_PATH)/Makefile)
