#use "-Os" flags.
use-flags := -Os
#Don't use "-O2" flags.
not-use-flags := -O2
KBUILD_CFLAGS := $(filter-out $(not-use-flags), $(KBUILD_CFLAGS))
KBUILD_CFLAGS += $(use-flags)
GCOV_PROFILE := n

subdir- += standby
obj-y	 += pm_tmp.o 

pm_tmp-y :=	mem_mapping.o               \
		mem_divlibc.o               \
		pm_of.o                     \
		mem_int.o                   \
		mem_clk.o                   \
		mem_key.o                   \
		mem_ir.o                   \
		mem_usb.o                   \
		mem_tmr.o                   \
		mem_tmstmp.o                \
		pm_debug.o                  \
		pm_debug_secure.o           \
		mem_hwspinlock.o            \
		pm.o                        \
		aw_pm.o			    \
		aw_pwr_dm.o                 

ifneq ($(strip $(CONFIG_ARCH_SUN8IW6) $(CONFIG_ARCH_SUN8IW8) $(CONFIG_ARCH_SUN50IW1P1)),)
pm_tmp-y +=	extended_standby-sun8iw6.o
else
pm_tmp-y +=	extended_standby.o
endif


	
