#ifndef ___YUV_SENSOR_H__T8EV5_OUT
#define ___YUV_SENSOR_H__T8EV5_OUT

#include <linux/ioctl.h>  /* For IOCTL macros */

/*-------------------------------------------Important---------------------------------------
 * for changing the SENSOR_NAME, you must need to change the owner of the device. For example
 * Please add /dev/mt9d115 0600 media camera in below file
 * ./device/nvidia/ventana/ueventd.ventana.rc
 * Otherwise, ioctl will get permission deny
 * -------------------------------------------Important--------------------------------------
*/


#ifdef __KERNEL__
#define T8EV5_FRONT_ID (1<<0)//bit 0 =1 front else back
#define MT9P111_TYPE_ID (1<<1)
#define T8EV5_TYPE_ID (1<<2)
struct yuv_t8ev5_sensor_platform_data {
	int led_en;
       int flash_en;
	unsigned int  flash_delay;
	int (*power_on)(u8 flag);
	int (*power_off)(u8 flag);
};
#endif /* __KERNEL__ */

#endif  /* __YUV_SENSOR_H__ */

