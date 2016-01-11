#ifeq ($(CONFIG_ARM64),y)
#obj-y += bmulib_64.o_shipped
#else
#obj-y += bmulib.o_shipped
#endif

#obj-y += bmulib.o

### build bmulib to bmulib.ko
obj-m += bmulibko.o
## use bmulib.oo to build ko
bmulibko-objs := bmulib.o
