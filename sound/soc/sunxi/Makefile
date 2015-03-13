# SUNXI platform Support

snd-sunxi-soc-objs := sunxi_dma.o
snd-sunxi-soc-objs += codec_utils.o
ifneq ($(CONFIG_SND_SOC_SUNXI_RW),)
snd-sunxi-soc-objs += sunxi_rw_func.o
endif
ifneq ($(CONFIG_SND_SOC_SUNXI_TDM),)
snd-sunxi-soc-objs += sunxi_tdm_utils.o
endif
obj-y += snd-sunxi-soc.o

#audiocodec

obj-$(CONFIG_SND_SOC_INTERNAL_AUDIOCODEC) += sunxi_sun50iw1codec.o
obj-$(CONFIG_SND_SOC_INTERNAL_I2S) += sunxi_i2s.o
obj-$(CONFIG_SND_SOC_AUDIO_CODEC_MACHINE) += sunxi_sndsun50iw1.o


#daudio0
obj-$(CONFIG_SND_SOC_VIRCODEC) += snddaudio.o
obj-$(CONFIG_SND_SOC_DAUDIO_PLATFORM) += sunxi_daudio.o
obj-$(CONFIG_SND_SOC_DAUDIO0_MACHINE) += sunxi-snddaudio0.o

#daudio1
obj-$(CONFIG_SND_SOC_DAUDIO1_MACHINE) += sunxi-snddaudio1.o

#hdmi
obj-$(CONFIG_SND_SUNXI_SOC_HDMIAUDIO) += sunxi_tdmhdmi.o
obj-$(CONFIG_SND_SUNXI_SOC_HDMIAUDIO) += hdmi.o
obj-$(CONFIG_SND_SUNXI_SOC_HDMIAUDIO) += sunxi-sndhdmi.o

#spdif
obj-$(CONFIG_SND_SUNXI_SOC_SPDIF) += sunxi_spdif.o
obj-$(CONFIG_SND_SUNXI_SOC_SPDIF) += sunxi-sndspdif.o

##DEBUG
ifeq ($(CONFIG_SUNXI_AUDIO_DEBUG), y)
    EXTRA_CFLAGS += -DDEBUG
endif