#
# Makefile for Bosch sensor driver.
#
obj-$(CONFIG_SENSORS_BMG)    += bmg160_driver.o bmg160.o
ifeq ($(CONFIG_SENSORS_BMG_FIFO),y)
	EXTRA_CFLAGS += -DBMG_USE_FIFO -DBMG_USE_BASIC_I2C_FUNC
endif
