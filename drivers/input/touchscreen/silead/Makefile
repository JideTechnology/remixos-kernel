
#ifeq ($(TARGET_PRODUCT),anzhen4_mrd7)
    obj-$(CONFIG_TOUCHSCREEN_SILEAD)	+= gsl_point_id.o gsl_ts_driver.o 
    $(obj)/gsl_point_id.o: $(srctree)/$(obj)/gsl_point_id
	    cp $(srctree)/$(obj)/gsl_point_id $(obj)/gsl_point_id.o
#else
#        obj-$(CONFIG_TOUCHSCREEN_SILEAD)	+= gsl_point_id.o gsl_ts_driver.o 
#        $(obj)/gsl_point_id.o: $(srctree)/$(obj)/gsl_point_id
#	    cp $(srctree)/$(obj)/gsl_point_id $(obj)/gsl_point_id.o
#endif
#obj-y += gsl_point_id.o gsl1688.o
#$(obj)/gsl_point_id.o: $(srctree)/$(obj)/gsl_point_id
#	cp $(srctree)/$(obj)/gsl_point_id $(obj)/gsl_point_id.o
